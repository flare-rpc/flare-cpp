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



// Date: Sun Jul 13 15:04:18 CST 2014
#include <inttypes.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>  // F_GETFD
#include <gtest/gtest.h>
#include <gflags/gflags.h>
#include "flare/base/gperftools_profiler.h"
#include "flare/base/time.h"
#include "flare/base/fd_utility.h"
#include "flare/strings/starts_with.h"
#include "flare/fiber/internal/unstable.h"
#include "flare/fiber/internal/schedule_group.h"
#include "flare/rpc/socket.h"
#include "flare/rpc/errno.pb.h"
#include "flare/rpc/acceptor.h"
#include "flare/rpc/policy/hulu_pbrpc_protocol.h"
#include "flare/rpc/policy/most_common_message.h"
#include "flare/rpc/policy/http_rpc_protocol.h"
#include "flare/rpc/nshead.h"
#include "flare/rpc/server.h"
#include "flare/rpc/channel.h"
#include "flare/rpc/controller.h"
#include "flare/fiber/this_fiber.h"
#include "health_check.pb.h"

#if defined(FLARE_PLATFORM_OSX)

#include <sys/event.h>

#endif

#define CONNECT_IN_KEEPWRITE 1;

namespace flare::fiber_internal {
    extern schedule_group *g_task_control;
}

namespace flare::rpc {
    DECLARE_int32(health_check_interval);
}

void EchoProcessHuluRequest(flare::rpc::InputMessageBase *msg_base);

int main(int argc, char *argv[]) {
    testing::InitGoogleTest(&argc, argv);
    google::ParseCommandLineFlags(&argc, &argv, true);
    flare::rpc::Protocol dummy_protocol =
            {flare::rpc::policy::ParseHuluMessage,
             flare::rpc::SerializeRequestDefault,
             flare::rpc::policy::PackHuluRequest,
             EchoProcessHuluRequest, EchoProcessHuluRequest,
             NULL, NULL, NULL,
             flare::rpc::CONNECTION_TYPE_ALL, "dummy_hulu"};
    EXPECT_EQ(0, RegisterProtocol((flare::rpc::ProtocolType) 30, dummy_protocol));
    return RUN_ALL_TESTS();
}

struct WaitData {
    fiber_token_t id;
    int error_code;
    std::string error_text;

    WaitData() : id(INVALID_FIBER_TOKEN), error_code(0) {}
};

int OnWaitIdReset(fiber_token_t id, void *data, int error_code,
                  const std::string &error_text) {
    static_cast<WaitData *>(data)->id = id;
    static_cast<WaitData *>(data)->error_code = error_code;
    static_cast<WaitData *>(data)->error_text = error_text;
    return fiber_token_unlock_and_destroy(id);
}

class SocketTest : public ::testing::Test {
protected:

    SocketTest() {
    };

    virtual ~SocketTest() {};

    virtual void SetUp() {
    };

    virtual void TearDown() {
    };
};

flare::rpc::Socket *global_sock = NULL;

class CheckRecycle : public flare::rpc::SocketUser {
    void BeforeRecycle(flare::rpc::Socket *s) {
        ASSERT_TRUE(global_sock);
        ASSERT_EQ(global_sock, s);
        global_sock = NULL;
        delete this;
    }
};

TEST_F(SocketTest, not_recycle_until_zero_nref) {
    std::cout << "sizeof(Socket)=" << sizeof(flare::rpc::Socket) << std::endl;
    int fds[2];
    ASSERT_EQ(0, socketpair(AF_UNIX, SOCK_STREAM, 0, fds));
    flare::rpc::SocketId id = 8888;
    flare::base::end_point dummy;
    ASSERT_EQ(0, str2endpoint("192.168.1.26:8080", &dummy));
    flare::rpc::SocketOptions options;
    options.fd = fds[1];
    options.remote_side = dummy;
    options.user = new CheckRecycle;
    ASSERT_EQ(0, flare::rpc::Socket::Create(options, &id));
    {
        flare::rpc::SocketUniquePtr s;
        ASSERT_EQ(0, flare::rpc::Socket::Address(id, &s));
        global_sock = s.get();
        ASSERT_TRUE(s.get());
        ASSERT_EQ(fds[1], s->fd());
        ASSERT_EQ(dummy, s->remote_side());
        ASSERT_EQ(id, s->id());
        ASSERT_EQ(0, s->SetFailed());
        ASSERT_EQ(s.get(), global_sock);
    }
    ASSERT_EQ((flare::rpc::Socket *) NULL, global_sock);
    close(fds[0]);

    flare::rpc::SocketUniquePtr ptr;
    ASSERT_EQ(-1, flare::rpc::Socket::Address(id, &ptr));
}

std::atomic<int> winner_count(0);
const int AUTH_ERR = -9;

void *auth_fighter(void *arg) {
    flare::fiber_sleep_for(10000);
    int auth_error = 0;
    flare::rpc::Socket *s = (flare::rpc::Socket *) arg;
    if (s->FightAuthentication(&auth_error) == 0) {
        winner_count.fetch_add(1);
        s->SetAuthentication(AUTH_ERR);
    } else {
        EXPECT_EQ(AUTH_ERR, auth_error);
    }
    return NULL;
}

TEST_F(SocketTest, authentication) {
    flare::rpc::SocketId id;
    flare::rpc::SocketOptions options;
    ASSERT_EQ(0, flare::rpc::Socket::Create(options, &id));
    flare::rpc::SocketUniquePtr s;
    ASSERT_EQ(0, flare::rpc::Socket::Address(id, &s));

    fiber_id_t th[64];
    for (size_t i = 0; i < FLARE_ARRAY_SIZE(th); ++i) {
        ASSERT_EQ(0, fiber_start_urgent(&th[i], NULL, auth_fighter, s.get()));
    }
    for (size_t i = 0; i < FLARE_ARRAY_SIZE(th); ++i) {
        ASSERT_EQ(0, fiber_join(th[i], NULL));
    }
    // Only one fighter wins
    ASSERT_EQ(1, winner_count.load());

    // Fight after signal is OK
    int auth_error = 0;
    ASSERT_NE(0, s->FightAuthentication(&auth_error));
    ASSERT_EQ(AUTH_ERR, auth_error);
    // Socket has been `SetFailed' when authentication failed
    ASSERT_TRUE(flare::rpc::Socket::Address(s->id(), NULL));
}

static std::atomic<int> g_called_seq(1);

class MyMessage : public flare::rpc::SocketMessage {
public:
    MyMessage(const char *str, size_t len, int *called = NULL)
            : _str(str), _len(len), _called(called) {}

private:

    flare::base::flare_status AppendAndDestroySelf(flare::cord_buf *out_buf, flare::rpc::Socket *) {
        out_buf->append(_str, _len);
        if (_called) {
            *_called = g_called_seq.fetch_add(1, std::memory_order_relaxed);
        }
        delete this;
        return flare::base::flare_status::OK();
    };
    const char *_str;
    size_t _len;
    int *_called;
};

class MyErrorMessage : public flare::rpc::SocketMessage {
public:
    explicit MyErrorMessage(const flare::base::flare_status &st) : _status(st) {}

private:

    flare::base::flare_status AppendAndDestroySelf(flare::cord_buf *, flare::rpc::Socket *) {
        return _status;
    };
    flare::base::flare_status _status;
};

TEST_F(SocketTest, single_threaded_write) {
    int fds[2];
    ASSERT_EQ(0, socketpair(AF_UNIX, SOCK_STREAM, 0, fds));
    flare::rpc::SocketId id = 8888;
    flare::base::end_point dummy;
    ASSERT_EQ(0, str2endpoint("192.168.1.26:8080", &dummy));
    flare::rpc::SocketOptions options;
    options.fd = fds[1];
    options.remote_side = dummy;
    options.user = new CheckRecycle;
    ASSERT_EQ(0, flare::rpc::Socket::Create(options, &id));
    {
        flare::rpc::SocketUniquePtr s;
        ASSERT_EQ(0, flare::rpc::Socket::Address(id, &s));
        global_sock = s.get();
        ASSERT_TRUE(s.get());
        ASSERT_EQ(fds[1], s->fd());
        ASSERT_EQ(dummy, s->remote_side());
        ASSERT_EQ(id, s->id());
        const int BATCH = 5;
        for (size_t i = 0; i < 20; ++i) {
            char buf[32 * BATCH];
            size_t len = snprintf(buf, sizeof(buf), "hello world! %lu", i);
            if (i % 4 == 0) {
                flare::rpc::SocketMessagePtr<MyMessage> msg(new MyMessage(buf, len));
                ASSERT_EQ(0, s->Write(msg));
            } else if (i % 4 == 1) {
                flare::rpc::SocketMessagePtr<MyErrorMessage> msg(
                        new MyErrorMessage(flare::base::flare_status(EINVAL, "Invalid input")));
                fiber_token_t wait_id;
                WaitData data;
                ASSERT_EQ(0, fiber_token_create2(&wait_id, &data, OnWaitIdReset));
                flare::rpc::Socket::WriteOptions wopt;
                wopt.id_wait = wait_id;
                ASSERT_EQ(0, s->Write(msg, &wopt));
                ASSERT_EQ(0, fiber_token_join(wait_id));
                ASSERT_EQ(wait_id.value, data.id.value);
                ASSERT_EQ(EINVAL, data.error_code);
                ASSERT_EQ("Invalid input", data.error_text);
                continue;
            } else if (i % 4 == 2) {
                int seq[BATCH] = {};
                flare::rpc::SocketMessagePtr<MyMessage> msgs[BATCH];
                // re-print the buffer.
                len = 0;
                for (int j = 0; j < BATCH; ++j) {
                    if (j % 2 == 0) {
                        // Empty message, should be skipped.
                        msgs[j].reset(new MyMessage(buf + len, 0, &seq[j]));
                    } else {
                        size_t sub_len = snprintf(
                                buf + len, sizeof(buf) - len, "hello world! %lu.%d", i, j);
                        msgs[j].reset(new MyMessage(buf + len, sub_len, &seq[j]));
                        len += sub_len;
                    }
                }
                for (size_t i = 0; i < BATCH; ++i) {
                    ASSERT_EQ(0, s->Write(msgs[i]));
                }
                for (int j = 1; j < BATCH; ++j) {
                    ASSERT_LT(seq[j - 1], seq[j]) << "j=" << j;
                }
            } else {
                flare::cord_buf src;
                src.append(buf);
                ASSERT_EQ(len, src.length());
                ASSERT_EQ(0, s->Write(&src));
                ASSERT_TRUE(src.empty());
            }
            char dest[sizeof(buf)];
            ASSERT_EQ(len, (size_t) read(fds[0], dest, sizeof(dest)));
            ASSERT_EQ(0, memcmp(buf, dest, len));
        }
        ASSERT_EQ(0, s->SetFailed());
    }
    ASSERT_EQ((flare::rpc::Socket *) NULL, global_sock);
    close(fds[0]);
}

void EchoProcessHuluRequest(flare::rpc::InputMessageBase *msg_base) {
    flare::rpc::DestroyingPtr<flare::rpc::policy::MostCommonMessage> msg(
            static_cast<flare::rpc::policy::MostCommonMessage *>(msg_base));
    flare::cord_buf buf;
    buf.append(msg->meta);
    buf.append(msg->payload);
    ASSERT_EQ(0, msg->socket()->Write(&buf));
}

class MyConnect : public flare::rpc::AppConnect {
public:
    MyConnect() : _done(NULL), _data(NULL), _called_start_connect(false) {}

    void StartConnect(const flare::rpc::Socket *,
                      void (*done)(int err, void *data),
                      void *data) {
        LOG(INFO) << "Start application-level connect";
        _done = done;
        _data = data;
        _called_start_connect = true;
    }

    void StopConnect(flare::rpc::Socket *) {
        LOG(INFO) << "Stop application-level connect";
    }

    void MakeConnectDone() {
        _done(0, _data);
    }

    bool is_start_connect_called() const { return _called_start_connect; }

private:

    void (*_done)(int err, void *data);

    void *_data;
    bool _called_start_connect;
};

TEST_F(SocketTest, single_threaded_connect_and_write) {
    // FIXME(gejun): Messenger has to be new otherwise quitting may crash.
    flare::rpc::Acceptor *messenger = new flare::rpc::Acceptor;
    const flare::rpc::InputMessageHandler pairs[] = {
            {flare::rpc::policy::ParseHuluMessage,
                    EchoProcessHuluRequest, NULL, NULL, "dummy_hulu"}
    };

    flare::base::end_point point(flare::base::IP_ANY, 7878);
    int listening_fd = tcp_listen(point);
    ASSERT_TRUE(listening_fd > 0);
    flare::base::make_non_blocking(listening_fd);
    ASSERT_EQ(0, messenger->AddHandler(pairs[0]));
    ASSERT_EQ(0, messenger->StartAccept(listening_fd, -1, NULL));

    flare::rpc::SocketId id = 8888;
    flare::rpc::SocketOptions options;
    options.remote_side = point;
    std::shared_ptr<MyConnect> my_connect = std::make_shared<MyConnect>();
    options.app_connect = my_connect;
    options.user = new CheckRecycle;
    ASSERT_EQ(0, flare::rpc::Socket::Create(options, &id));
    {
        flare::rpc::SocketUniquePtr s;
        ASSERT_EQ(0, flare::rpc::Socket::Address(id, &s));
        global_sock = s.get();
        ASSERT_TRUE(s.get());
        ASSERT_EQ(-1, s->fd());
        ASSERT_EQ(point, s->remote_side());
        ASSERT_EQ(id, s->id());
        for (size_t i = 0; i < 20; ++i) {
            char buf[64];
            const size_t meta_len = 4;
            *(uint32_t *) (buf + 12) = *(uint32_t *) "Meta";
            const size_t len = snprintf(buf + 12 + meta_len,
                                        sizeof(buf) - 12 - meta_len,
                                        "hello world! %lu", i);
            memcpy(buf, "HULU", 4);
            // HULU uses host byte order directly...
            *(uint32_t *) (buf + 4) = len + meta_len;
            *(uint32_t *) (buf + 8) = meta_len;

            int called = 0;
            if (i % 2 == 0) {
                flare::rpc::SocketMessagePtr<MyMessage> msg(
                        new MyMessage(buf, 12 + meta_len + len, &called));
                ASSERT_EQ(0, s->Write(msg));
            } else {
                flare::cord_buf src;
                src.append(buf, 12 + meta_len + len);
                ASSERT_EQ(12 + meta_len + len, src.length());
                ASSERT_EQ(0, s->Write(&src));
                ASSERT_TRUE(src.empty());
            }
            if (i == 0) {
                // connection needs to be established at first time.
                // Should be intentionally blocked in app_connect.
                flare::fiber_sleep_for(10000);
                ASSERT_TRUE(my_connect->is_start_connect_called());
                ASSERT_LT(0, s->fd()); // already tcp connected
                ASSERT_EQ(0, called); // request is not serialized yet.
                my_connect->MakeConnectDone();
                ASSERT_LT(0, called); // serialized
            }
            int64_t start_time = flare::base::gettimeofday_us();
            while (s->fd() < 0) {
                flare::fiber_sleep_for(1000);
                ASSERT_LT(flare::base::gettimeofday_us(), start_time + 1000000L) << "Too long!";
            }
#if defined(FLARE_PLATFORM_LINUX)
            ASSERT_EQ(0, fiber_fd_wait(s->fd(), EPOLLIN));
#elif defined(FLARE_PLATFORM_OSX)
            ASSERT_EQ(0, fiber_fd_wait(s->fd(), EVFILT_READ));
#endif
            char dest[sizeof(buf)];
            ASSERT_EQ(meta_len + len, (size_t) read(s->fd(), dest, sizeof(dest)));
            ASSERT_EQ(0, memcmp(buf + 12, dest, meta_len + len));
        }
        ASSERT_EQ(0, s->SetFailed());
    }
    ASSERT_EQ((flare::rpc::Socket *) NULL, global_sock);
    // The id is invalid.
    flare::rpc::SocketUniquePtr ptr;
    ASSERT_EQ(-1, flare::rpc::Socket::Address(id, &ptr));

    messenger->StopAccept(0);
    ASSERT_EQ(-1, messenger->listened_fd());
    ASSERT_EQ(-1, fcntl(listening_fd, F_GETFD));
    ASSERT_EQ(EBADF, errno);
}

#define NUMBER_WIDTH 16

struct WriterArg {
    size_t times;
    size_t offset;
    flare::rpc::SocketId socket_id;
};

void *FailedWriter(void *void_arg) {
    WriterArg *arg = static_cast<WriterArg *>(void_arg);
    flare::rpc::SocketUniquePtr sock;
    if (flare::rpc::Socket::Address(arg->socket_id, &sock) < 0) {
        printf("Fail to address SocketId=%" PRIu64 "\n", arg->socket_id);
        return NULL;
    }
    char buf[32];
    for (size_t i = 0; i < arg->times; ++i) {
        fiber_token_t id;
        EXPECT_EQ(0, fiber_token_create(&id, NULL, NULL));
        snprintf(buf, sizeof(buf), "%0" FLARE_SYMBOLSTR(NUMBER_WIDTH) "lu",
                 i + arg->offset);
        flare::cord_buf src;
        src.append(buf);
        flare::rpc::Socket::WriteOptions wopt;
        wopt.id_wait = id;
        sock->Write(&src, &wopt);
        EXPECT_EQ(0, fiber_token_join(id));
        // Only the first connect can see ECONNREFUSED and then
        // calls `SetFailed' making others' error_code=EINVAL
        //EXPECT_EQ(ECONNREFUSED, error_code);
    }
    return NULL;
}

TEST_F(SocketTest, fail_to_connect) {
    const size_t REP = 10;
    flare::base::end_point point(flare::base::IP_ANY, 7563/*not listened*/);
    flare::rpc::SocketId id = 8888;
    flare::rpc::SocketOptions options;
    options.remote_side = point;
    options.user = new CheckRecycle;
    ASSERT_EQ(0, flare::rpc::Socket::Create(options, &id));
    {
        flare::rpc::SocketUniquePtr s;
        ASSERT_EQ(0, flare::rpc::Socket::Address(id, &s));
        global_sock = s.get();
        ASSERT_TRUE(s.get());
        ASSERT_EQ(-1, s->fd());
        ASSERT_EQ(point, s->remote_side());
        ASSERT_EQ(id, s->id());
        pthread_t th[8];
        WriterArg args[FLARE_ARRAY_SIZE(th)];
        for (size_t i = 0; i < FLARE_ARRAY_SIZE(th); ++i) {
            args[i].times = REP;
            args[i].offset = i * REP;
            args[i].socket_id = id;
            ASSERT_EQ(0, pthread_create(&th[i], NULL, FailedWriter, &args[i]));
        }
        for (size_t i = 0; i < FLARE_ARRAY_SIZE(th); ++i) {
            ASSERT_EQ(0, pthread_join(th[i], NULL));
        }
        ASSERT_EQ(-1, s->SetFailed());  // already SetFailed
        ASSERT_EQ(-1, s->fd());
    }
    // KeepWrite is possibly still running.
    int64_t start_time = flare::base::gettimeofday_us();
    while (global_sock != NULL) {
        flare::fiber_sleep_for(1000);
        ASSERT_LT(flare::base::gettimeofday_us(), start_time + 1000000L) << "Too long!";
    }
    ASSERT_EQ(-1, flare::rpc::Socket::Status(id));
    // The id is invalid.
    flare::rpc::SocketUniquePtr ptr;
    ASSERT_EQ(-1, flare::rpc::Socket::Address(id, &ptr));
}

TEST_F(SocketTest, not_health_check_when_nref_hits_0) {
    flare::rpc::SocketId id = 8888;
    flare::base::end_point point(flare::base::IP_ANY, 7584/*not listened*/);
    flare::rpc::SocketOptions options;
    options.remote_side = point;
    options.user = new CheckRecycle;
    options.health_check_interval_s = 1/*s*/;
    ASSERT_EQ(0, flare::rpc::Socket::Create(options, &id));
    {
        flare::rpc::SocketUniquePtr s;
        ASSERT_EQ(0, flare::rpc::Socket::Address(id, &s));
        global_sock = s.get();
        ASSERT_TRUE(s.get());
        ASSERT_EQ(-1, s->fd());
        ASSERT_EQ(point, s->remote_side());
        ASSERT_EQ(id, s->id());

        char buf[64];
        const size_t meta_len = 4;
        *(uint32_t *) (buf + 12) = *(uint32_t *) "Meta";
        const size_t len = snprintf(buf + 12 + meta_len,
                                    sizeof(buf) - 12 - meta_len,
                                    "hello world!");
        memcpy(buf, "HULU", 4);
        // HULU uses host byte order directly...
        *(uint32_t *) (buf + 4) = len + meta_len;
        *(uint32_t *) (buf + 8) = meta_len;
        flare::cord_buf src;
        src.append(buf, 12 + meta_len + len);
        ASSERT_EQ(12 + meta_len + len, src.length());
#ifdef CONNECT_IN_KEEPWRITE
        fiber_token_t wait_id;
        WaitData data;
        ASSERT_EQ(0, fiber_token_create2(&wait_id, &data, OnWaitIdReset));
        flare::rpc::Socket::WriteOptions wopt;
        wopt.id_wait = wait_id;
        ASSERT_EQ(0, s->Write(&src, &wopt));
        ASSERT_EQ(0, fiber_token_join(wait_id));
        ASSERT_EQ(wait_id.value, data.id.value);
        ASSERT_EQ(ECONNREFUSED, data.error_code);
        ASSERT_TRUE(flare::starts_with(data.error_text,
                                       "Fail to connect "));
#else
        ASSERT_EQ(-1, s->Write(&src));
        ASSERT_EQ(ECONNREFUSED, errno);
#endif
        ASSERT_TRUE(src.empty());
        ASSERT_EQ(-1, s->fd());
    }
    // HealthCheckThread is possibly still running. Spin until global_sock
    // is NULL(set in CheckRecycle::BeforeRecycle). Notice that you should
    // not spin until Socket::Status(id) becomes -1 and assert global_sock
    // to be NULL because invalidating id happens before calling BeforeRecycle.
    const int64_t start_time = flare::base::gettimeofday_us();
    while (global_sock != NULL) {
        flare::fiber_sleep_for(1000);
        ASSERT_LT(flare::base::gettimeofday_us(), start_time + 1000000L);
    }
    ASSERT_EQ(-1, flare::rpc::Socket::Status(id));
}

class HealthCheckTestServiceImpl : public test::HealthCheckTestService {
public:
    HealthCheckTestServiceImpl()
            : _sleep_flag(true) {}

    virtual ~HealthCheckTestServiceImpl() {}

    virtual void default_method(google::protobuf::RpcController *cntl_base,
                                const test::HealthCheckRequest *request,
                                test::HealthCheckResponse *response,
                                google::protobuf::Closure *done) {
        flare::rpc::ClosureGuard done_guard(done);
        flare::rpc::Controller *cntl = (flare::rpc::Controller *) cntl_base;
        if (_sleep_flag) {
            flare::fiber_sleep_for(510000 /* 510ms, a little bit longer than the default
                                     timeout of health check rpc */);
        }
        cntl->response_attachment().append("OK");
    }

    bool _sleep_flag;
};

TEST_F(SocketTest, app_level_health_check) {
    int old_health_check_interval = flare::rpc::FLAGS_health_check_interval;
    google::SetCommandLineOption("health_check_path", "/HealthCheckTestService");
    google::SetCommandLineOption("health_check_interval", "1");

    flare::base::end_point point(flare::base::IP_ANY, 7777);
    flare::rpc::ChannelOptions options;
    options.protocol = "http";
    options.max_retry = 0;
    flare::rpc::Channel channel;
    ASSERT_EQ(0, channel.Init(point, &options));
    {
        flare::rpc::Controller cntl;
        cntl.http_request().uri() = "/";
        channel.CallMethod(NULL, &cntl, NULL, NULL, NULL);
        EXPECT_TRUE(cntl.Failed());
        ASSERT_EQ(ECONNREFUSED, cntl.ErrorCode());
    }

    // 2s to make sure remote is connected by HealthCheckTask and enter the
    // sending-rpc state. Because the remote is not down, so hc rpc would keep
    // sending.
    int listening_fd = tcp_listen(point);
    flare::fiber_sleep_for(2000000);

    // 2s to make sure HealthCheckTask find socket is failed and correct impl
    // should trigger next round of hc
    close(listening_fd);
    flare::fiber_sleep_for(2000000);

    flare::rpc::Server server;
    HealthCheckTestServiceImpl hc_service;
    ASSERT_EQ(0, server.AddService(&hc_service, flare::rpc::SERVER_DOESNT_OWN_SERVICE));
    ASSERT_EQ(0, server.Start(point, NULL));

    for (int i = 0; i < 4; ++i) {
        // although ::connect would succeed, the stall in hc_service makes
        // the health check rpc fail.
        flare::rpc::Controller cntl;
        cntl.http_request().uri() = "/";
        channel.CallMethod(NULL, &cntl, NULL, NULL, NULL);
        ASSERT_EQ(EHOSTDOWN, cntl.ErrorCode());
        flare::fiber_sleep_for(1000000 /*1s*/);
    }
    hc_service._sleep_flag = false;
    flare::fiber_sleep_for(2000000 /* a little bit longer than hc rpc timeout + hc interval */);
    // should recover now
    {
        flare::rpc::Controller cntl;
        cntl.http_request().uri() = "/";
        channel.CallMethod(NULL, &cntl, NULL, NULL, NULL);
        ASSERT_FALSE(cntl.Failed());
        ASSERT_GT(cntl.response_attachment().size(), (size_t) 0);
    }

    google::SetCommandLineOption("health_check_path", "");
    char hc_buf[8];
    snprintf(hc_buf, sizeof(hc_buf), "%d", old_health_check_interval);
    google::SetCommandLineOption("health_check_interval", hc_buf);
}

TEST_F(SocketTest, health_check) {
    // FIXME(gejun): Messenger has to be new otherwise quitting may crash.
    flare::rpc::Acceptor *messenger = new flare::rpc::Acceptor;

    flare::rpc::SocketId id = 8888;
    flare::base::end_point point(flare::base::IP_ANY, 7878);
    const int kCheckInteval = 1;
    flare::rpc::SocketOptions options;
    options.remote_side = point;
    options.user = new CheckRecycle;
    options.health_check_interval_s = kCheckInteval/*s*/;
    ASSERT_EQ(0, flare::rpc::Socket::Create(options, &id));
    flare::rpc::SocketUniquePtr s;
    ASSERT_EQ(0, flare::rpc::Socket::Address(id, &s));

    global_sock = s.get();
    ASSERT_TRUE(s.get());
    ASSERT_EQ(-1, s->fd());
    ASSERT_EQ(point, s->remote_side());
    ASSERT_EQ(id, s->id());
    int32_t nref = -1;
    ASSERT_EQ(0, flare::rpc::Socket::Status(id, &nref));
    ASSERT_EQ(2, nref);

    char buf[64];
    const size_t meta_len = 4;
    *(uint32_t *) (buf + 12) = *(uint32_t *) "Meta";
    const size_t len = snprintf(buf + 12 + meta_len,
                                sizeof(buf) - 12 - meta_len,
                                "hello world!");
    memcpy(buf, "HULU", 4);
    // HULU uses host byte order directly...
    *(uint32_t *) (buf + 4) = len + meta_len;
    *(uint32_t *) (buf + 8) = meta_len;
    const bool use_my_message = (flare::base::fast_rand_less_than(2) == 0);
    flare::rpc::SocketMessagePtr<MyMessage> msg;
    int appended_msg = 0;
    flare::cord_buf src;
    if (use_my_message) {
        LOG(INFO) << "Use MyMessage";
        msg.reset(new MyMessage(buf, 12 + meta_len + len, &appended_msg));
    } else {
        src.append(buf, 12 + meta_len + len);
        ASSERT_EQ(12 + meta_len + len, src.length());
    }
#ifdef CONNECT_IN_KEEPWRITE
    fiber_token_t wait_id;
    WaitData data;
    ASSERT_EQ(0, fiber_token_create2(&wait_id, &data, OnWaitIdReset));
    flare::rpc::Socket::WriteOptions wopt;
    wopt.id_wait = wait_id;
    if (use_my_message) {
        ASSERT_EQ(0, s->Write(msg, &wopt));
    } else {
        ASSERT_EQ(0, s->Write(&src, &wopt));
    }
    ASSERT_EQ(0, fiber_token_join(wait_id));
    ASSERT_EQ(wait_id.value, data.id.value);
    ASSERT_EQ(ECONNREFUSED, data.error_code);
    ASSERT_TRUE(flare::starts_with(data.error_text,
                                   "Fail to connect "));
    if (use_my_message) {
        ASSERT_TRUE(appended_msg);
    }
#else
        if (use_my_message) {
            ASSERT_EQ(-1, s->Write(msg));
        } else {
            ASSERT_EQ(-1, s->Write(&src));
        }
        ASSERT_EQ(ECONNREFUSED, errno);
#endif
    ASSERT_TRUE(src.empty());
    ASSERT_EQ(-1, s->fd());
    ASSERT_TRUE(global_sock);
    flare::rpc::SocketUniquePtr invalid_ptr;
    ASSERT_EQ(-1, flare::rpc::Socket::Address(id, &invalid_ptr));
    ASSERT_EQ(1, flare::rpc::Socket::Status(id));

    const flare::rpc::InputMessageHandler pairs[] = {
            {flare::rpc::policy::ParseHuluMessage,
                    EchoProcessHuluRequest, NULL, NULL, "dummy_hulu"}
    };

    int listening_fd = tcp_listen(point);
    ASSERT_TRUE(listening_fd > 0);
    flare::base::make_non_blocking(listening_fd);
    ASSERT_EQ(0, messenger->AddHandler(pairs[0]));
    ASSERT_EQ(0, messenger->StartAccept(listening_fd, -1, NULL));

    int64_t start_time = flare::base::gettimeofday_us();
    nref = -1;
    while (flare::rpc::Socket::Status(id, &nref) != 0) {
        flare::fiber_sleep_for(1000);
        ASSERT_LT(flare::base::gettimeofday_us(),
                  start_time + kCheckInteval * 1000000L + 100000L/*100ms*/);
    }
    //ASSERT_EQ(2, nref);
    ASSERT_TRUE(global_sock);

    int fd = 0;
    {
        flare::rpc::SocketUniquePtr ptr;
        ASSERT_EQ(0, flare::rpc::Socket::Address(id, &ptr));
        ASSERT_NE(0, ptr->fd());
        fd = ptr->fd();
    }

    // SetFailed again, should reconnect and succeed soon.
    ASSERT_EQ(0, s->SetFailed());
    ASSERT_EQ(fd, s->fd());
    start_time = flare::base::gettimeofday_us();
    while (flare::rpc::Socket::Status(id) != 0) {
        flare::fiber_sleep_for(1000);
        ASSERT_LT(flare::base::gettimeofday_us(), start_time + 1000000L);
    }
    ASSERT_TRUE(global_sock);

    {
        flare::rpc::SocketUniquePtr ptr;
        ASSERT_EQ(0, flare::rpc::Socket::Address(id, &ptr));
        ASSERT_NE(0, ptr->fd());
    }

    s.release()->Dereference();

    // Must stop messenger before SetFailed the id otherwise HealthCheckThread
    // still has chance to get reconnected and revive the id.
    messenger->StopAccept(0);
    ASSERT_EQ(-1, messenger->listened_fd());
    ASSERT_EQ(-1, fcntl(listening_fd, F_GETFD));
    ASSERT_EQ(EBADF, errno);

    ASSERT_EQ(0, flare::rpc::Socket::SetFailed(id));
    // HealthCheckThread is possibly still addressing the Socket.
    start_time = flare::base::gettimeofday_us();
    while (global_sock != NULL) {
        flare::fiber_sleep_for(1000);
        ASSERT_LT(flare::base::gettimeofday_us(), start_time + 1000000L);
    }
    ASSERT_EQ(-1, flare::rpc::Socket::Status(id));
    // The id is invalid.
    flare::rpc::SocketUniquePtr ptr;
    ASSERT_EQ(-1, flare::rpc::Socket::Address(id, &ptr));
}

void *Writer(void *void_arg) {
    WriterArg *arg = static_cast<WriterArg *>(void_arg);
    flare::rpc::SocketUniquePtr sock;
    if (flare::rpc::Socket::Address(arg->socket_id, &sock) < 0) {
        printf("Fail to address SocketId=%" PRIu64 "\n", arg->socket_id);
        return NULL;
    }
    char buf[32];
    for (size_t i = 0; i < arg->times; ++i) {
        snprintf(buf, sizeof(buf), "%0" FLARE_SYMBOLSTR(NUMBER_WIDTH) "lu",
                 i + arg->offset);
        flare::cord_buf src;
        src.append(buf);
        if (sock->Write(&src) != 0) {
            if (errno == flare::rpc::EOVERCROWDED) {
                // The buf is full, sleep a while and retry.
                flare::fiber_sleep_for(1000);
                --i;
                continue;
            }
            printf("Fail to write into SocketId=%" PRIu64 ", %s\n",
                   arg->socket_id, flare_error());
            break;
        }
    }
    return NULL;
}

TEST_F(SocketTest, multi_threaded_write) {
    const size_t REP = 20000;
    int fds[2];
    for (int k = 0; k < 2; ++k) {
        printf("Round %d\n", k + 1);
        ASSERT_EQ(0, socketpair(AF_UNIX, SOCK_STREAM, 0, fds));
        pthread_t th[8];
        WriterArg args[FLARE_ARRAY_SIZE(th)];
        std::vector<size_t> result;
        result.reserve(FLARE_ARRAY_SIZE(th) * REP);

        flare::rpc::SocketId id = 8888;
        flare::base::end_point dummy;
        ASSERT_EQ(0, str2endpoint("192.168.1.26:8080", &dummy));
        flare::rpc::SocketOptions options;
        options.fd = fds[1];
        options.remote_side = dummy;
        options.user = new CheckRecycle;
        ASSERT_EQ(0, flare::rpc::Socket::Create(options, &id));
        flare::rpc::SocketUniquePtr s;
        ASSERT_EQ(0, flare::rpc::Socket::Address(id, &s));
        s->_ssl_state = flare::rpc::SSL_OFF;
        global_sock = s.get();
        ASSERT_TRUE(s.get());
        ASSERT_EQ(fds[1], s->fd());
        ASSERT_EQ(dummy, s->remote_side());
        ASSERT_EQ(id, s->id());
        flare::base::make_non_blocking(fds[0]);

        for (size_t i = 0; i < FLARE_ARRAY_SIZE(th); ++i) {
            args[i].times = REP;
            args[i].offset = i * REP;
            args[i].socket_id = id;
            ASSERT_EQ(0, pthread_create(&th[i], NULL, Writer, &args[i]));
        }

        if (k == 1) {
            printf("sleep 100ms to block writers\n");
            flare::fiber_sleep_for(100000);
        }

        flare::IOPortal dest;
        const int64_t start_time = flare::base::gettimeofday_us();
        for (;;) {
            ssize_t nr = dest.append_from_file_descriptor(fds[0], 32768);
            if (nr < 0) {
                if (errno == EINTR) {
                    continue;
                }
                if (EAGAIN != errno) {
                    ASSERT_EQ(EAGAIN, errno) << flare_error();
                }
                flare::fiber_sleep_for(1000);
                if (flare::base::gettimeofday_us() >= start_time + 2000000L) {
                    LOG(FATAL) << "Wait too long!";
                    break;
                }
                continue;
            }
            while (dest.length() >= NUMBER_WIDTH) {
                char buf[NUMBER_WIDTH + 1];
                dest.copy_to(buf, NUMBER_WIDTH);
                buf[sizeof(buf) - 1] = 0;
                result.push_back(strtol(buf, NULL, 10));
                dest.pop_front(NUMBER_WIDTH);
            }
            if (result.size() >= REP * FLARE_ARRAY_SIZE(th)) {
                break;
            }
        }
        for (size_t i = 0; i < FLARE_ARRAY_SIZE(th); ++i) {
            ASSERT_EQ(0, pthread_join(th[i], NULL));
        }
        ASSERT_TRUE(dest.empty());
        flare::fiber_internal::g_task_control->print_rq_sizes(std::cout);
        std::cout << std::endl;

        ASSERT_EQ(REP * FLARE_ARRAY_SIZE(th), result.size())
                                    << "write_head=" << s->_write_head;
        std::sort(result.begin(), result.end());
        result.resize(std::unique(result.begin(),
                                  result.end()) - result.begin());
        ASSERT_EQ(REP * FLARE_ARRAY_SIZE(th), result.size());
        ASSERT_EQ(0UL, *result.begin());
        ASSERT_EQ(REP * FLARE_ARRAY_SIZE(th) - 1, *(result.end() - 1));

        ASSERT_EQ(0, s->SetFailed());
        s.release()->Dereference();
        ASSERT_EQ((flare::rpc::Socket *) NULL, global_sock);
        close(fds[0]);
    }
}

void *FastWriter(void *void_arg) {
    WriterArg *arg = static_cast<WriterArg *>(void_arg);
    flare::rpc::SocketUniquePtr sock;
    if (flare::rpc::Socket::Address(arg->socket_id, &sock) < 0) {
        printf("Fail to address SocketId=%" PRIu64 "\n", arg->socket_id);
        return NULL;
    }
    char buf[] = "hello reader side!";
    int64_t begin_ts = flare::base::cpuwide_time_us();
    int64_t nretry = 0;
    size_t c = 0;
    for (; c < arg->times; ++c) {
        flare::cord_buf src;
        src.append(buf, 16);
        if (sock->Write(&src) != 0) {
            if (errno == flare::rpc::EOVERCROWDED) {
                // The buf is full, sleep a while and retry.
                flare::fiber_sleep_for(1000);
                --c;
                ++nretry;
                continue;
            }
            printf("Fail to write into SocketId=%" PRIu64 ", %s\n",
                   arg->socket_id, flare_error());
            break;
        }
    }
    int64_t end_ts = flare::base::cpuwide_time_us();
    int64_t total_time = end_ts - begin_ts;
    printf("total=%ld count=%ld nretry=%ld\n",
           (long) total_time * 1000 / c, (long) c, (long) nretry);
    return NULL;
}

struct ReaderArg {
    int fd;
    size_t nread;
};

void *reader(void *void_arg) {
    ReaderArg *arg = static_cast<ReaderArg *>(void_arg);
    const size_t LEN = 32768;
    char *buf = (char *) malloc(LEN);
    while (1) {
        ssize_t nr = read(arg->fd, buf, LEN);
        if (nr < 0) {
            printf("Fail to read, %m\n");
            return NULL;
        } else if (nr == 0) {
            printf("Far end closed\n");
            return NULL;
        }
        arg->nread += nr;
    }
    return NULL;
}

TEST_F(SocketTest, multi_threaded_write_perf) {
    const size_t REP = 1000000000;
    int fds[2];
    ASSERT_EQ(0, socketpair(AF_UNIX, SOCK_STREAM, 0, fds));
    fiber_id_t th[3];
    WriterArg args[FLARE_ARRAY_SIZE(th)];

    flare::rpc::SocketId id = 8888;
    flare::base::end_point dummy;
    ASSERT_EQ(0, str2endpoint("192.168.1.26:8080", &dummy));
    flare::rpc::SocketOptions options;
    options.fd = fds[1];
    options.remote_side = dummy;
    options.user = new CheckRecycle;
    ASSERT_EQ(0, flare::rpc::Socket::Create(options, &id));
    flare::rpc::SocketUniquePtr s;
    ASSERT_EQ(0, flare::rpc::Socket::Address(id, &s));
    s->_ssl_state = flare::rpc::SSL_OFF;
    ASSERT_EQ(2, flare::rpc::NRefOfVRef(s->_versioned_ref));
    global_sock = s.get();
    ASSERT_TRUE(s.get());
    ASSERT_EQ(fds[1], s->fd());
    ASSERT_EQ(dummy, s->remote_side());
    ASSERT_EQ(id, s->id());

    for (size_t i = 0; i < FLARE_ARRAY_SIZE(th); ++i) {
        args[i].times = REP;
        args[i].offset = i * REP;
        args[i].socket_id = id;
        fiber_start_background(&th[i], NULL, FastWriter, &args[i]);
    }

    pthread_t rth;
    ReaderArg reader_arg = {fds[0], 0};
    pthread_create(&rth, NULL, reader, &reader_arg);

    flare::base::stop_watcher tm;
    ProfilerStart("write.prof");
    const uint64_t old_nread = reader_arg.nread;
    tm.start();
    sleep(2);
    tm.stop();
    const uint64_t new_nread = reader_arg.nread;
    ProfilerStop();

    printf("tp=%" PRIu64 "M/s\n", (new_nread - old_nread) / tm.u_elapsed());

    for (size_t i = 0; i < FLARE_ARRAY_SIZE(th); ++i) {
        args[i].times = 0;
    }
    for (size_t i = 0; i < FLARE_ARRAY_SIZE(th); ++i) {
        ASSERT_EQ(0, fiber_join(th[i], NULL));
    }
    ASSERT_EQ(0, s->SetFailed());
    s.release()->Dereference();
    pthread_join(rth, NULL);
    ASSERT_EQ((flare::rpc::Socket *) NULL, global_sock);
    close(fds[0]);
}
