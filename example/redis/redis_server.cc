
#include <flare/rpc/server.h>
#include <flare/rpc/redis.h>
#include <flare/base/crc32c.h>
#include <gflags/gflags.h>
#include <unordered_map>

#include "flare/times/time.h"

DEFINE_int32(port,
6379, "TCP Port of this server");

class RedisServiceImpl : public flare::rpc::RedisService {
public:
    bool Set(const std::string &key, const std::string &value) {
        int slot = flare::crc32c().extend(key) % kHashSlotNum;
        _mutex[slot].lock();
        _map[slot][key] = value;
        _mutex[slot].unlock();
        return true;
    }

    bool Get(const std::string &key, std::string *value) {
        int slot = flare::crc32c().extend(key) % kHashSlotNum;
        _mutex[slot].lock();
        auto it = _map[slot].find(key);
        if (it == _map[slot].end()) {
            _mutex[slot].unlock();
            return false;
        }
        *value = it->second;
        _mutex[slot].unlock();
        return true;
    }

private:
    const static int kHashSlotNum = 32;
    std::unordered_map<std::string, std::string> _map[kHashSlotNum];
    std::mutex _mutex[kHashSlotNum];
};

class GetCommandHandler : public flare::rpc::RedisCommandHandler {
public:
    explicit GetCommandHandler(RedisServiceImpl *rsimpl)
            : _rsimpl(rsimpl) {}

    flare::rpc::RedisCommandHandlerResult Run(const std::vector<std::string_view> &args,
                                              flare::rpc::RedisReply *output,
                                              bool /*flush_batched*/) override {
        if (args.size() != 2ul) {
            output->FormatError("Expect 1 arg for 'get', actually %lu", args.size() - 1);
            return flare::rpc::REDIS_CMD_HANDLED;
        }
        const std::string key(args[1].data(), args[1].size());
        std::string value;
        if (_rsimpl->Get(key, &value)) {
            output->SetString(value);
        } else {
            output->SetNullString();
        }
        return flare::rpc::REDIS_CMD_HANDLED;
    }

private:
    RedisServiceImpl *_rsimpl;
};

class SetCommandHandler : public flare::rpc::RedisCommandHandler {
public:
    explicit SetCommandHandler(RedisServiceImpl *rsimpl)
            : _rsimpl(rsimpl) {}

    flare::rpc::RedisCommandHandlerResult Run(const std::vector<std::string_view> &args,
                                              flare::rpc::RedisReply *output,
                                              bool /*flush_batched*/) override {
        if (args.size() != 3ul) {
            output->FormatError("Expect 2 args for 'set', actually %lu", args.size() - 1);
            return flare::rpc::REDIS_CMD_HANDLED;
        }
        const std::string key(args[1].data(), args[1].size());
        const std::string value(args[2].data(), args[2].size());
        _rsimpl->Set(key, value);
        output->SetStatus("OK");
        return flare::rpc::REDIS_CMD_HANDLED;
    }

private:
    RedisServiceImpl *_rsimpl;
};

int main(int argc, char *argv[]) {
    google::ParseCommandLineFlags(&argc, &argv, true);
    RedisServiceImpl *rsimpl = new RedisServiceImpl;
    rsimpl->AddCommandHandler("get", new GetCommandHandler(rsimpl));
    rsimpl->AddCommandHandler("set", new SetCommandHandler(rsimpl));

    flare::rpc::Server server;
    flare::rpc::ServerOptions server_options;
    server_options.redis_service = rsimpl;
    if (server.Start(FLAGS_port, &server_options) != 0) {
        FLARE_LOG(ERROR) << "Fail to start server";
        return -1;
    }
    server.RunUntilAskedToQuit();
    return 0;
}
