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

// A server to receive EchoRequest and send back EchoResponse.

#include <fcntl.h>  // O_RDONLY
#include <vector>
#include <random>
#include <chrono>
#include <gflags/gflags.h>
#include "flare/times/time.h"
#include "flare/log/logging.h"
#include <flare/base/strings.h>
#include <flare/base/string_splitter.h>
#include <flare/rpc/server.h>
#include "echo.pb.h"

DEFINE_bool(send_attachment, false, "Carry attachment along with response");
DEFINE_int32(port, 8004, "TCP Port of this server");
DEFINE_int32(idle_timeout_s, -1, "Connection will be closed if there is no "
             "read/write operations during the last `idle_timeout_s'");
DEFINE_int32(logoff_ms, 2000, "Maximum duration of server's LOGOFF state "
             "(waiting for client to close connection before server stops)");
DEFINE_int32(max_concurrency, 0, "Limit of request processing in parallel");
DEFINE_int32(server_num, 1, "Number of servers");
DEFINE_string(sleep_us, "", "Sleep so many microseconds before responding");
DEFINE_bool(spin, false, "spin rather than sleep");
DEFINE_double(exception_ratio, 0.1, "Percentage of irregular latencies");
DEFINE_double(min_ratio, 0.2, "min_sleep / sleep_us");
DEFINE_double(max_ratio, 10, "max_sleep / sleep_us");

// Your implementation of example::EchoService
class EchoServiceImpl : public example::EchoService {
public:
    EchoServiceImpl() : _index(0) {}
    virtual ~EchoServiceImpl() {};
    void set_index(size_t index, int64_t sleep_us) { 
        _index = index; 
        _sleep_us = sleep_us;
    }
    virtual void Echo(google::protobuf::RpcController* cntl_base,
                      const example::EchoRequest* request,
                      example::EchoResponse* response,
                      google::protobuf::Closure* done) {
        flare::rpc::ClosureGuard done_guard(done);
        flare::rpc::Controller* cntl =
            static_cast<flare::rpc::Controller*>(cntl_base);
        if (_sleep_us > 0) {
            double delay = _sleep_us;
            const double a = FLAGS_exception_ratio * 0.5;
            if (a >= 0.0001) {
                unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
                std::default_random_engine e(seed);
                std::uniform_real_distribution<double> distrReal(0,5);
                double x = distrReal(e);
                if (x < a) {
                    const double min_sleep_us = FLAGS_min_ratio * _sleep_us;
                    delay = min_sleep_us + (_sleep_us - min_sleep_us) * x / a;
                } else if (x + a > 1) {
                    const double max_sleep_us = FLAGS_max_ratio * _sleep_us;
                    delay = _sleep_us + (max_sleep_us - _sleep_us) * (x + a - 1) / a;
                }
            }
            if (FLAGS_spin) {
                int64_t end_time = flare::get_current_time_micros() + (int64_t)delay;
                while (flare::get_current_time_micros() < end_time) {}
            } else {
                flare::fiber_sleep_for((int64_t)delay);
            }
        }

        // Fill response.
        response->set_value(request->value() + 1);
        if (FLAGS_send_attachment) {
            // Set attachment which is wired to network directly instead of
            // being serialized into protobuf messages.
            cntl->response_attachment().append("bar");
        }
        _nreq << 1;
    }

    size_t num_requests() const { return _nreq.get_value(); }

private:
    size_t _index;
    int64_t _sleep_us;
    flare::variable::Adder<size_t> _nreq;
};

int main(int argc, char* argv[]) {
    // Parse gflags. We recommend you to use gflags as well.
    google::ParseCommandLineFlags(&argc, &argv, true);

    if (FLAGS_server_num <= 0) {
        FLARE_LOG(ERROR) << "server_num must be positive";
        return -1;
    }

    // We need multiple servers in this example.
    flare::rpc::Server* servers = new flare::rpc::Server[FLAGS_server_num];

    flare::StringSplitter sp(FLAGS_sleep_us.c_str(), ',');
    std::vector<int64_t> sleep_list;
    for (; sp; ++sp) {
        sleep_list.push_back(strtoll(sp.field(), NULL, 10));
    }
    if (sleep_list.empty()) {
        sleep_list.push_back(0);
    }

    // Instance of your services.
    EchoServiceImpl* echo_service_impls = new EchoServiceImpl[FLAGS_server_num];

    // Add the service into servers. Notice the second parameter, because the
    // service is put on stack, we don't want server to delete it, otherwise
    // use flare::rpc::SERVER_OWNS_SERVICE.
    for (int i = 0; i < FLAGS_server_num; ++i) {
        int64_t sleep_us = sleep_list[(size_t)i < sleep_list.size() ? i : (sleep_list.size() - 1)];
        echo_service_impls[i].set_index(i, sleep_us);
        servers[i].set_version(flare::string_printf(
                    "example/multi_threaded_echo_fns_c++[%d]", i));
        if (servers[i].AddService(&echo_service_impls[i], 
                                  flare::rpc::SERVER_DOESNT_OWN_SERVICE) != 0) {
            FLARE_LOG(ERROR) << "Fail to add service";
            return -1;
        }
        // Start the server.
        flare::rpc::ServerOptions options;
        options.idle_timeout_sec = FLAGS_idle_timeout_s;
        options.max_concurrency = FLAGS_max_concurrency;
        const int port = FLAGS_port + i;
        if (servers[i].Start(port, &options) != 0) {
            FLARE_LOG(ERROR) << "Fail to start EchoServer";
            return -1;
        }

        // Intended no truncate so that multiple servers can be added to list
        int fd = open("./server_list", O_APPEND | O_WRONLY | O_CREAT, 0666);
        if (fd < 0) {
            FLARE_PLOG(ERROR) << "Fail to open server_list";
            return -1;
        }
        char buf[64];
        int nw = snprintf(buf, sizeof(buf), "%s:%d\n", flare::base::my_ip_cstr(), port);
        if (write(fd, buf, nw) != nw) {
            FLARE_LOG(ERROR) << "Fail to fully write int fd=" << fd;
        }
        close(fd);
    }

    // Service logic are running in separate worker threads, for main thread,
    // we don't have much to do, just spinning.
    std::vector<size_t> last_num_requests(FLAGS_server_num);
    while (!flare::rpc::IsAskedToQuit()) {
        sleep(1);

        size_t cur_total = 0;
        for (int i = 0; i < FLAGS_server_num; ++i) {
            const size_t current_num_requests =
                    echo_service_impls[i].num_requests();
            size_t diff = current_num_requests - last_num_requests[i];
            cur_total += diff;
            last_num_requests[i] = current_num_requests;
            FLARE_LOG(INFO) << "S[" << i << "]=" << diff << ' ' << noflush;
        }
        FLARE_LOG(INFO) << "[total=" << cur_total << ']';
    }

    // Don't forget to stop and join the server otherwise still-running
    // worker threads may crash your program. Clients will have/ at most
    // `FLAGS_logoff_ms' to close their connections. If some connections
    // still remains after `FLAGS_logoff_ms', they will be closed by force.
    for (int i = 0; i < FLAGS_server_num; ++i) {
        servers[i].Stop(FLAGS_logoff_ms);
    }
    for (int i = 0; i < FLAGS_server_num; ++i) {
        servers[i].Join();
    }
    delete [] servers;
    delete [] echo_service_impls;
    return 0;
}
