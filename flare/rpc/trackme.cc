// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.


#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include "flare/base/fast_rand.h"
#include "flare/rpc/log.h"
#include "flare/rpc/channel.h"
#include "flare/rpc/trackme.pb.h"
#include "flare/rpc/policy/hasher.h"
#include "flare/files/readline_file.h"
#include "flare/strings/str_format.h"
#include "flare/strings/strip.h"
#include "flare/strings/numbers.h"


namespace flare::rpc {

    DEFINE_string(trackme_server, "", "Where the TrackMe requests are sent to");

    static const int32_t TRACKME_MIN_INTERVAL = 30;
    static const int32_t TRACKME_MAX_INTERVAL = 600;
    static int32_t s_trackme_interval = TRACKME_MIN_INTERVAL;
    // Protecting global vars on trackme
    static pthread_mutex_t s_trackme_mutex = PTHREAD_MUTEX_INITIALIZER;
    // For contacting with trackme_server.
    static Channel *s_trackme_chan = nullptr;
    // Any server address in this process.
    static std::string *s_trackme_addr = nullptr;

    // Information of bugs.
    // Notice that this structure may be a combination of all affected bugs.
    // Namely `severity' is severity of the worst bug and `error_text' is
    // combination of descriptions of all bugs. Check tools/trackme_server
    // for impl. details.
    struct BugInfo {
        TrackMeSeverity severity;
        std::string error_text;

        bool operator==(const BugInfo &bug) const {
            return severity == bug.severity && error_text == bug.error_text;
        }
    };

    // If a bug was shown, its info was stored in this var as well so that we
    // can avoid showing the same bug repeatly.
    static BugInfo *g_bug_info = nullptr;
    // The timestamp(microseconds) that we sent TrackMeRequest.
    static int64_t s_trackme_last_time = 0;

// version of RPC.
// Since the code for getting FLARE_RPC_REVISION often fails,
// FLARE_RPC_REVISION must be defined to string and be converted to number
// within our code.
// The code running before main() may see g_rpc_version=0, should be OK.
#if defined(FLARE_RPC_REVISION)
    const int64_t g_rpc_version = atoll(FLARE_RPC_REVISION);
#else
    const int64_t g_rpc_version = 0;
#endif

    int read_jpaas_host_port(int container_port) {
        const uid_t uid = getuid();
        struct passwd *pw = getpwuid(uid);
        if (pw == nullptr) {
            RPC_VLOG << "Fail to get password file entry of uid=" << uid;
            return -1;
        }
        std::string JPAAS_LOG_PATH =  flare::string_format("{}/jpaas_run/logs/env.log", pw->pw_dir);
        flare::readline_file file;
        auto fs = file.open(JPAAS_LOG_PATH);
        if (!fs.is_ok()) {
            RPC_VLOG << "Fail to open `" << JPAAS_LOG_PATH << '\'';
            return -1;
        }
        int host_port = -1;
        std::string prefix = flare::string_format( "JPAAS_HOST_PORT_{}=", container_port);
        auto lines = file.lines();
        for (auto line : lines) {
            line = flare::strip_suffix(line, "\n");
            if (flare::starts_with(line, prefix)) {
                line = line.substr(prefix.size());
                auto r = flare::simple_atoi(line, &host_port);
                FLARE_UNUSED(r);
                break;
            }
        }
        RPC_VLOG_IF(host_port < 0) << "No entry starting with `" << prefix << "' found";
        return host_port;
    }

    // Called in server.cpp
    void SetTrackMeAddress(flare::base::end_point pt) {
        FLARE_SCOPED_LOCK(s_trackme_mutex);
        if (s_trackme_addr == nullptr) {
            // JPAAS has NAT capabilities, read its log to figure out the open port
            // accessible from outside.
            const int jpaas_port = read_jpaas_host_port(pt.port);
            if (jpaas_port > 0) {
                RPC_VLOG << "Use jpaas_host_port=" << jpaas_port
                         << " instead of jpaas_container_port=" << pt.port;
                pt.port = jpaas_port;
            }
            s_trackme_addr = new std::string(flare::base::endpoint2str(pt).c_str());
        }
    }

    static void HandleTrackMeResponse(Controller *cntl, TrackMeResponse *res) {
        if (cntl->Failed()) {
            RPC_VLOG << "Fail to access " << FLAGS_trackme_server << ", " << cntl->ErrorText();
        } else {
            BugInfo cur_info;
            cur_info.severity = res->severity();
            cur_info.error_text = res->error_text();
            bool already_reported = false;
            {
                FLARE_SCOPED_LOCK(s_trackme_mutex);
                if (g_bug_info != nullptr && *g_bug_info == cur_info) {
                    // we've shown the bug.
                    already_reported = true;
                } else {
                    // save the bug.
                    if (g_bug_info == nullptr) {
                        g_bug_info = new BugInfo(cur_info);
                    } else {
                        *g_bug_info = cur_info;
                    }
                }
            }
            if (!already_reported) {
                switch (res->severity()) {
                    case TrackMeOK:
                        break;
                    case TrackMeFatal:
                        FLARE_LOG(ERROR) << "Your flare (r" << g_rpc_version
                                         << ") is affected by: " << res->error_text();
                        break;
                    case TrackMeWarning:
                        FLARE_LOG(WARNING) << "Your flare (r" << g_rpc_version
                                           << ") is affected by: " << res->error_text();
                        break;
                    default:
                        FLARE_LOG(WARNING) << "Unknown severity=" << res->severity();
                        break;
                }
            }
            if (res->has_new_interval()) {
                // We can't fully trust the result from trackme_server which may
                // have bugs. Make sure the reporting interval is not too short or
                // too long
                int32_t new_interval = res->new_interval();
                new_interval = std::max(new_interval, TRACKME_MIN_INTERVAL);
                new_interval = std::min(new_interval, TRACKME_MAX_INTERVAL);
                if (new_interval != s_trackme_interval) {
                    s_trackme_interval = new_interval;
                    RPC_VLOG << "Update s_trackme_interval to " << new_interval;
                }
            }
        }
        delete cntl;
        delete res;
    }

    static void TrackMeNow(std::unique_lock<pthread_mutex_t> &mu) {
        if (s_trackme_addr == nullptr) {
            return;
        }
        if (s_trackme_chan == nullptr) {
            Channel *chan = new(std::nothrow) Channel;
            if (chan == nullptr) {
                FLARE_LOG(FATAL) << "Fail to new trackme channel";
                return;
            }
            ChannelOptions opt;
            // keep #connections on server-side low
            opt.connection_type = CONNECTION_TYPE_SHORT;
            if (chan->Init(FLAGS_trackme_server.c_str(), "c_murmurhash", &opt) != 0) {
                FLARE_LOG(WARNING) << "Fail to connect to " << FLAGS_trackme_server;
                delete chan;
                return;
            }
            s_trackme_chan = chan;
        }
        mu.unlock();
        TrackMeService_Stub stub(s_trackme_chan);
        TrackMeRequest req;
        req.set_rpc_version(g_rpc_version);
        req.set_server_addr(*s_trackme_addr);
        TrackMeResponse *res = new TrackMeResponse;
        Controller *cntl = new Controller;
        cntl->set_request_code(policy::MurmurHash32(s_trackme_addr->data(), s_trackme_addr->size()));
        google::protobuf::Closure *done =
                ::flare::rpc::NewCallback(&HandleTrackMeResponse, cntl, res);
        stub.TrackMe(cntl, &req, res, done);
    }

// Called in global.cpp
// [Thread-safe] supposed to be called in low frequency.
    void TrackMe() {
        if (FLAGS_trackme_server.empty()) {
            return;
        }
        int64_t now = flare::get_current_time_micros();
        std::unique_lock<pthread_mutex_t> mu(s_trackme_mutex);
        if (s_trackme_last_time == 0) {
            // Delay the first ping randomly within s_trackme_interval. This
            // protects trackme_server from ping storms.
            s_trackme_last_time =
                    now + flare::base::fast_rand_less_than(s_trackme_interval) * 1000000L;
        }
        if (now > s_trackme_last_time + 1000000L * s_trackme_interval) {
            s_trackme_last_time = now;
            return TrackMeNow(mu);
        }
    }

} // namespace flare::rpc
