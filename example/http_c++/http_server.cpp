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

// A server to receive HttpRequest and send back HttpResponse.

#include <gflags/gflags.h>
#include "flare/log/logging.h"
#include "flare/bootstrap/bootstrap.h"
#include <flare/rpc/server.h>
#include <flare/rpc/restful.h>
#include "http.pb.h"

DEFINE_int32(port, 8010, "TCP Port of this server");
DEFINE_int32(idle_timeout_s, -1, "Connection will be closed if there is no "
                                 "read/write operations during the last `idle_timeout_s'");
DEFINE_int32(logoff_ms, 2000, "Maximum duration of server's LOGOFF state "
                              "(waiting for client to close connection before server stops)");

DEFINE_string(certificate, "cert.pem", "Certificate file path to enable SSL");
DEFINE_string(private_key, "key.pem", "Private key file path to enable SSL");
DEFINE_string(ciphers, "", "Cipher suite used for SSL connections");

namespace example {

// Service with static path.
    class HttpServiceImpl : public HttpService {
    public:
        HttpServiceImpl() {};

        virtual ~HttpServiceImpl() {};

        void Echo(google::protobuf::RpcController *cntl_base,
                  const HttpRequest *,
                  HttpResponse *,
                  google::protobuf::Closure *done) {
            // This object helps you to call done->Run() in RAII style. If you need
            // to process the request asynchronously, pass done_guard.release().
            flare::rpc::ClosureGuard done_guard(done);

            flare::rpc::Controller *cntl =
                    static_cast<flare::rpc::Controller *>(cntl_base);
            // Fill response.
            cntl->http_response().set_content_type("text/plain");
            flare::io::cord_buf_builder os;
            os << "queries:";
            for (flare::rpc::URI::QueryIterator it = cntl->http_request().uri().QueryBegin();
                 it != cntl->http_request().uri().QueryEnd(); ++it) {
                os << ' ' << it->first << '=' << it->second;
            }
            os << "\nbody: " << cntl->request_attachment() << '\n';
            os.move_to(cntl->response_attachment());
        }
    };

// Service with dynamic path.
    class FileServiceImpl : public FileService {
    public:
        FileServiceImpl() {};

        virtual ~FileServiceImpl() {};

        struct Args {
            flare::container::intrusive_ptr<flare::rpc::ProgressiveAttachment> pa;
        };

        static void *SendLargeFile(void *raw_args) {
            std::unique_ptr<Args> args(static_cast<Args *>(raw_args));
            if (args->pa == NULL) {
                LOG(ERROR) << "ProgressiveAttachment is NULL";
                return NULL;
            }
            for (int i = 0; i < 100; ++i) {
                char buf[16];
                int len = snprintf(buf, sizeof(buf), "part_%d ", i);
                args->pa->Write(buf, len);

                // sleep a while to send another part.
                flare::this_fiber::fiber_sleep_for(10000);
            }
            return NULL;
        }

        void default_method(google::protobuf::RpcController *cntl_base,
                            const HttpRequest *,
                            HttpResponse *,
                            google::protobuf::Closure *done) {
            flare::rpc::ClosureGuard done_guard(done);
            flare::rpc::Controller *cntl =
                    static_cast<flare::rpc::Controller *>(cntl_base);
            const std::string &filename = cntl->http_request().unresolved_path();
            if (filename == "largefile") {
                // Send the "largefile" with ProgressiveAttachment.
                std::unique_ptr<Args> args(new Args);
                args->pa = cntl->CreateProgressiveAttachment();
                fiber_id_t th;
                bthread_start_background(&th, NULL, SendLargeFile, args.release());
            } else {
                cntl->response_attachment().append("Getting file: ");
                cntl->response_attachment().append(filename);
            }
        }
    };

    // Restful service. (The service implementation is exactly same with regular
    // services, the difference is that you need to pass a `restful_mappings'
    // when adding the service into server).
    class QueueServiceImpl : public example::QueueService {
    public:
        QueueServiceImpl() {};

        virtual ~QueueServiceImpl() {};

        void start(google::protobuf::RpcController *cntl_base,
                   const HttpRequest *,
                   HttpResponse *,
                   google::protobuf::Closure *done) {
            flare::rpc::ClosureGuard done_guard(done);
            flare::rpc::Controller *cntl =
                    static_cast<flare::rpc::Controller *>(cntl_base);
            cntl->response_attachment().append("queue started");
        }

        void stop(google::protobuf::RpcController *cntl_base,
                  const HttpRequest *,
                  HttpResponse *,
                  google::protobuf::Closure *done) {
            flare::rpc::ClosureGuard done_guard(done);
            flare::rpc::Controller *cntl =
                    static_cast<flare::rpc::Controller *>(cntl_base);
            cntl->response_attachment().append("queue stopped");
        }

        void getstats(google::protobuf::RpcController *cntl_base,
                      const HttpRequest *,
                      HttpResponse *,
                      google::protobuf::Closure *done) {
            flare::rpc::ClosureGuard done_guard(done);
            flare::rpc::Controller *cntl =
                    static_cast<flare::rpc::Controller *>(cntl_base);
            const std::string &unresolved_path = cntl->http_request().unresolved_path();
            if (unresolved_path.empty()) {
                cntl->response_attachment().append("Require a name after /stats");
            } else {
                cntl->response_attachment().append("Get stats: ");
                cntl->response_attachment().append(unresolved_path);
            }
        }
    };

}  // namespace example

int main(int argc, char *argv[]) {

    flare::bootstrap::bootstrap_init(argc, argv);
    flare::bootstrap::run_bootstrap();
    // Generally you only need one Server.
    flare::rpc::Server server;

    example::HttpServiceImpl http_svc;
    example::FileServiceImpl file_svc;
    example::QueueServiceImpl queue_svc;

    // Add services into server. Notice the second parameter, because the
    // service is put on stack, we don't want server to delete it, otherwise
    // use flare::rpc::SERVER_OWNS_SERVICE.
    if (server.AddService(&http_svc,
                          flare::rpc::SERVER_DOESNT_OWN_SERVICE) != 0) {
        LOG(ERROR) << "Fail to add http_svc";
        return -1;
    }
    if (server.AddService(&file_svc,
                          flare::rpc::SERVER_DOESNT_OWN_SERVICE) != 0) {
        LOG(ERROR) << "Fail to add file_svc";
        return -1;
    }
    if (server.AddService(&queue_svc,
                          flare::rpc::SERVER_DOESNT_OWN_SERVICE,
                          "/v1/queue/start   => start,"
                          "/v1/queue/stop    => stop,"
                          "/v1/queue/stats/* => getstats") != 0) {
        LOG(ERROR) << "Fail to add queue_svc";
        return -1;
    }

    // Start the server.
    flare::rpc::ServerOptions options;
    options.idle_timeout_sec = FLAGS_idle_timeout_s;
    options.mutable_ssl_options()->default_cert.certificate = FLAGS_certificate;
    options.mutable_ssl_options()->default_cert.private_key = FLAGS_private_key;
    options.mutable_ssl_options()->ciphers = FLAGS_ciphers;
    if (server.Start(FLAGS_port, &options) != 0) {
        LOG(ERROR) << "Fail to start HttpServer";
        return -1;
    }

    // Wait until Ctrl-C is pressed, then Stop() and Join() the server.
    server.RunUntilAskedToQuit();
    flare::bootstrap::run_finalizers();
    return 0;
}
