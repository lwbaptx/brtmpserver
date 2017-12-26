#ifndef BRTMPSERVER_RTMPSERVICE_HPP
#define BRTMPSERVER_RTMPSERVICE_HPP

#include <set>
#include <map>
#include <gflags/gflags.h>
#include <butil/logging.h>
#include <brpc/server.h>
#include <brpc/rtmp.pb.h>
#include <brpc/rtmp.h>

namespace brtmpserver {

class RtmpServerStreamImpl : public brpc::RtmpServerStream {
public:
    typedef std::map<std::string, RtmpServerStreamImpl*> PublishersMapType;

public:
    RtmpServerStreamImpl();
    virtual ~RtmpServerStreamImpl();
    virtual void OnPlay(const brpc::RtmpPlayOptions& options,
                        butil::Status* status,
                        google::protobuf::Closure* done);
    virtual void OnPublish(const std::string& stream_name,
                            brpc::RtmpPublishType publish_type,
                            butil::Status* status,
                           google::protobuf::Closure* done);
    virtual void OnAudioMessage(brpc::RtmpAudioMessage* msg);
    virtual void OnVideoMessage(brpc::RtmpVideoMessage* msg);
    virtual int SendAudioMessage(const brpc::RtmpAudioMessage& msg);
    virtual int SendVideoMessage(const brpc::RtmpVideoMessage& msg);


private:
    std::set<RtmpServerStreamImpl*> _players;
    RtmpServerStreamImpl* _publisher;
    brpc::RtmpVideoMessage _avc_sequence_header;
    brpc::RtmpAudioMessage _aac_sequence_header;
    bool _aac_header_sended;
    bool _avc_header_sended;
    std::string _stream_name;
    bool _is_publish;
    static PublishersMapType _publishers;
};

class RtmpServiceImpl : public brpc::RtmpService {
public:
    RtmpServiceImpl();
    virtual ~RtmpServiceImpl();
    virtual brpc::RtmpServerStream* NewStream(const brpc::RtmpConnectRequest&);
};

}  // namespace brtmpserver

#endif