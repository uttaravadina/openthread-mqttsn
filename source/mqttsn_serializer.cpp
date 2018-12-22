/*
 *  Copyright (c) 2018, Vit Holasek
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

#include <string.h>
#include "mqttsn_serializer.hpp"
#include "MQTTSNPacket.h"

namespace ot {

namespace Mqttsn {

static int32_t PacketDecode(const unsigned char* aData, size_t aLength)
{
    int lenlen = 0;
    int datalen = 0;

    lenlen = MQTTSNPacket_decode(const_cast<unsigned char*>(aData), aLength, &datalen);
    if (datalen != static_cast<int>(aLength))
    {
        return MQTTSNPACKET_READ_ERROR;
    }
    return aData[lenlen]; // return the packet type
}

otError MessageBase::DeserializeMessageType(const uint8_t* aBuffer, int32_t aBufferLength, MessageType* aMessageType)
{
    int32_t type = PacketDecode(aBuffer, aBufferLength);
    if (type == MQTTSNPACKET_READ_ERROR)
    {
        return OT_ERROR_FAILED;
    }
    *aMessageType = static_cast<MessageType>(type);
    return OT_ERROR_NONE;
}

otError AdvertiseMessage::Serialize(uint8_t* aBuffer, uint8_t aBufferLength, int32_t* aLength) const
{
    int32_t length = MQTTSNSerialize_advertise(aBuffer, aBufferLength, mGatewayId, mDuration);
    if (length <= 0)
    {
        return OT_ERROR_FAILED;
    }
    *aLength = length;
    return OT_ERROR_NONE;
}

otError AdvertiseMessage::Deserialize(const uint8_t* aBuffer, int32_t aBufferLength)
{
    if (MQTTSNDeserialize_advertise(&mGatewayId, &mDuration, const_cast<unsigned char*>(aBuffer), aBufferLength) != 1)
    {
        return OT_ERROR_FAILED;
    }
    return OT_ERROR_NONE;
}

otError SearchGwMessage::Serialize(uint8_t* aBuffer, uint8_t aBufferLength, int32_t* aLength) const
{
    int32_t length = MQTTSNSerialize_searchgw(aBuffer, aBufferLength, mRadius);
    if (length <= 0)
    {
        return OT_ERROR_FAILED;
    }
    *aLength = length;
    return OT_ERROR_NONE;
}

otError SearchGwMessage::Deserialize(const uint8_t* aBuffer, int32_t aBufferLength)
{
    if (MQTTSNDeserialize_searchgw(&mRadius, const_cast<unsigned char*>(aBuffer), aBufferLength) != 1)
    {
        return OT_ERROR_FAILED;
    }
    return OT_ERROR_NONE;
}

otError GwInfoMessage::Serialize(uint8_t* aBuffer, uint8_t aBufferLength, int32_t* aLength) const
{
    Ip6::Address::InfoString addressString = mAddress.ToString();
    char* address = const_cast<char*>(addressString.AsCString());
    int32_t length = MQTTSNSerialize_gwinfo(aBuffer, aBufferLength, mGatewayId, addressString.GetLength(), reinterpret_cast<unsigned char*>(address));
    if (length <= 0)
    {
        return OT_ERROR_FAILED;
    }
    *aLength = length;
    return OT_ERROR_NONE;
}

otError GwInfoMessage::Deserialize(const uint8_t* aBuffer, int32_t aBufferLength)
{
    otError error = OT_ERROR_NONE;
    unsigned short addressLength;
    unsigned char* address;
    Ip6::Address::InfoString addressString;
    int32_t length = MQTTSNDeserialize_gwinfo(&mGatewayId, &addressLength, &address, const_cast<unsigned char*>(aBuffer), aBufferLength);
    VerifyOrExit(length > 0, error = OT_ERROR_FAILED);
    SuccessOrExit(addressString.Set("%.*s", static_cast<int32_t>(addressLength), address));
    SuccessOrExit(error = mAddress.FromString(addressString.AsCString()));

exit:
    return error;
}

otError ConnectMessage::Serialize(uint8_t* aBuffer, uint8_t aBufferLength, int32_t* aLength) const
{
    MQTTSNPacket_connectData options = MQTTSNPacket_connectData_initializer;
    MQTTSNString clientId = MQTTSNString_initializer;
    clientId.cstring = const_cast<char *>(mClientId.AsCString());
    options.clientID = clientId;
    options.duration = mDuration;
    options.cleansession = static_cast<unsigned char>(mCleanSessionFlag);
    int32_t length = MQTTSNSerialize_connect(aBuffer, aBufferLength, &options);
    if (length <= 0)
    {
        return OT_ERROR_FAILED;
    }
    *aLength = length;
    return OT_ERROR_NONE;
}

otError ConnectMessage::Deserialize(const uint8_t* aBuffer, int32_t aBufferLength)
{
    otError error = OT_ERROR_NONE;
    MQTTSNPacket_connectData data = MQTTSNPacket_connectData_initializer;
    VerifyOrExit(MQTTSNDeserialize_connect(&data, const_cast<unsigned char*>(aBuffer), aBufferLength) == 1, error = OT_ERROR_FAILED);
    mCleanSessionFlag = static_cast<bool>(data.cleansession);
    mWillFlag = static_cast<bool>(data.willFlag);
    mDuration = data.duration;
    SuccessOrExit(error = mClientId.Set("%.*s", data.clientID.lenstring.len, data.clientID.lenstring.data));

exit:
    return error;
}

otError ConnackMessage::Serialize(uint8_t* aBuffer, uint8_t aBufferLength, int32_t* aLength) const
{
    int32_t length = MQTTSNSerialize_connack(aBuffer, aBufferLength, mReturnCode);
    if (length <= 0)
    {
        return OT_ERROR_FAILED;
    }
    *aLength = length;
    return OT_ERROR_NONE;
}

otError ConnackMessage::Deserialize(const uint8_t* aBuffer, int32_t aBufferLength)
{
    int code;
    if (MQTTSNDeserialize_connack(&code, const_cast<unsigned char*>(aBuffer), aBufferLength) != 1)
    {
        return OT_ERROR_FAILED;
    }
    mReturnCode = static_cast<ReturnCode>(code);
    return OT_ERROR_NONE;
}

otError RegisterMessage::Serialize(uint8_t* aBuffer, uint8_t aBufferLength, int32_t* aLength) const
{
    MQTTSNString topicName = MQTTSNString_initializer;
    topicName.cstring = const_cast<char*>(mTopicName.AsCString());
    int32_t length = MQTTSNSerialize_register(aBuffer, aBufferLength, static_cast<unsigned short>(mTopicId), mMessageId, &topicName);
    if (length <= 0)
    {
        return OT_ERROR_FAILED;
    }
    *aLength = length;
    return OT_ERROR_NONE;
}

otError RegisterMessage::Deserialize(const uint8_t* aBuffer, int32_t aBufferLength)
{
    otError error = OT_ERROR_NONE;
    unsigned short topicId;
    MQTTSNString topicName = MQTTSNString_initializer;
    if (MQTTSNDeserialize_register(&topicId, &mMessageId, &topicName, const_cast<unsigned char*>(aBuffer), aBufferLength) != 1)
    {
        return OT_ERROR_FAILED;
    }
    mTopicId = static_cast<TopicId>(topicId);
    SuccessOrExit(error = mTopicName.Set("%.*s", topicName.lenstring.len, topicName.lenstring.data));

exit:
    return error;
}

otError RegackMessage::Serialize(uint8_t* aBuffer, uint8_t aBufferLength, int32_t* aLength) const
{
    int32_t length = MQTTSNSerialize_regack(aBuffer, aBufferLength, static_cast<unsigned char>(mTopicId),
        mMessageId, static_cast<unsigned char>(mReturnCode));
    if (length <= 0)
    {
        return OT_ERROR_FAILED;
    }
    *aLength = length;
    return OT_ERROR_NONE;
}

otError RegackMessage::Deserialize(const uint8_t* aBuffer, int32_t aBufferLength)
{
    unsigned short topicId;
    unsigned char code;
    if (MQTTSNDeserialize_regack(&topicId, &mMessageId, &code, const_cast<unsigned char*>(aBuffer), aBufferLength) != 1)
    {
        return OT_ERROR_FAILED;
    }
    mTopicId = static_cast<TopicId>(topicId);
    mReturnCode = static_cast<ReturnCode>(code);
    return OT_ERROR_NONE;
}

otError PublishMessage::Serialize(uint8_t* aBuffer, uint8_t aBufferLength, int32_t* aLength) const
{
    MQTTSN_topicid topicId;
    memset(&topicId, 0, sizeof(topicId));
    topicId.type = MQTTSN_TOPIC_TYPE_NORMAL;
    topicId.data.id = static_cast<unsigned short>(mTopicId);
    int32_t length = MQTTSNSerialize_publish(aBuffer, aBufferLength, static_cast<unsigned char>(mDupFlag), static_cast<unsigned char>(mQos),
        static_cast<unsigned char>(mRetainedFlag), mMessageId, topicId, const_cast<unsigned char*>(mPayload), mPayloadLength);
    if (length <= 0)
    {
        return OT_ERROR_FAILED;
    }
    *aLength = length;
    return OT_ERROR_NONE;
}

otError PublishMessage::Deserialize(const uint8_t* aBuffer, int32_t aBufferLength)
{
    unsigned char dup;
    int qos;
    unsigned char retained;
    MQTTSN_topicid topicId;
    unsigned char* payload;
    int payloadLength;
    memset(&topicId, 0, sizeof(topicId));
    if (MQTTSNDeserialize_publish(&dup, &qos, &retained, &mMessageId, &topicId,
        &payload, &payloadLength, const_cast<unsigned char*>(aBuffer), aBufferLength) != 1)
    {
        return OT_ERROR_FAILED;
    }
    mDupFlag = static_cast<bool>(dup);
    mRetainedFlag = static_cast<bool>(retained);
    mQos = static_cast<Qos>(qos);
    mTopicId = static_cast<TopicId>(topicId.data.id);
    mPayload = payload;
    mPayloadLength = static_cast<int32_t>(payloadLength);
    return OT_ERROR_NONE;
}

otError PubackMessage::Serialize(uint8_t* aBuffer, uint8_t aBufferLength, int32_t* aLength) const
{
    int32_t length = MQTTSNSerialize_puback(aBuffer, aBufferLength, static_cast<unsigned char>(mTopicId),
        mMessageId, static_cast<unsigned char>(mReturnCode));
    if (length <= 0)
    {
        return OT_ERROR_FAILED;
    }
    *aLength = length;
    return OT_ERROR_NONE;
}

otError PubackMessage::Deserialize(const uint8_t* aBuffer, int32_t aBufferLength)
{
    unsigned char code;
    unsigned short topicId;
    if (MQTTSNDeserialize_puback(&topicId, &mMessageId, &code, const_cast<unsigned char*>(aBuffer), aBufferLength) != 1)
    {
        return OT_ERROR_FAILED;
    }
    mReturnCode = static_cast<ReturnCode>(code);
    mTopicId = static_cast<TopicId>(topicId);
    return OT_ERROR_NONE;
}

otError PubcompMessage::Serialize(uint8_t* aBuffer, uint8_t aBufferLength, int32_t* aLength) const
{
    int32_t length = MQTTSNSerialize_pubcomp(aBuffer, aBufferLength, mMessageId);
    if (length <= 0)
    {
        return OT_ERROR_FAILED;
    }
    *aLength = length;
    return OT_ERROR_NONE;
}

otError PubcompMessage::Deserialize(const uint8_t* aBuffer, int32_t aBufferLength)
{
    return OT_ERROR_NOT_IMPLEMENTED;
}

otError PubrecMessage::Serialize(uint8_t* aBuffer, uint8_t aBufferLength, int32_t* aLength) const
{
    int32_t length = MQTTSNSerialize_pubrec(aBuffer, aBufferLength, mMessageId);
    if (length <= 0)
    {
        return OT_ERROR_FAILED;
    }
    *aLength = length;
    return OT_ERROR_NONE;
}

otError PubrecMessage::Deserialize(const uint8_t* aBuffer, int32_t aBufferLength)
{
    return OT_ERROR_NOT_IMPLEMENTED;
}

otError PubrelMessage::Serialize(uint8_t* aBuffer, uint8_t aBufferLength, int32_t* aLength) const
{
    int32_t length = MQTTSNSerialize_pubrel(aBuffer, aBufferLength, mMessageId);
    if (length <= 0)
    {
        return OT_ERROR_FAILED;
    }
    *aLength = length;
    return OT_ERROR_NONE;
}

otError PubrelMessage::Deserialize(const uint8_t* aBuffer, int32_t aBufferLength)
{
    return OT_ERROR_NOT_IMPLEMENTED;
}

otError SubscribeMessage::Serialize(uint8_t* aBuffer, uint8_t aBufferLength, int32_t* aLength) const
{
    MQTTSN_topicid topicId;
    memset(&topicId, 0, sizeof(topicId));
    topicId.type = MQTTSN_TOPIC_TYPE_NORMAL;
    topicId.data.long_.name = const_cast<char*>(mTopicName.AsCString());
    topicId.data.long_.len = mTopicName.GetLength();
    int32_t length = MQTTSNSerialize_subscribe(aBuffer, aBufferLength, static_cast<unsigned char>(mDupFlag),
        static_cast<int>(mQos), mMessageId, &topicId);
    if (length <= 0)
    {
        return OT_ERROR_FAILED;
    }
    *aLength = length;
    return OT_ERROR_NONE;
}

otError SubscribeMessage::Deserialize(const uint8_t* aBuffer, int32_t aBufferLength)
{
    otError error = OT_ERROR_NONE;
    unsigned char dup;
    int qos;
    MQTTSN_topicid topicId;
    memset(&topicId, 0, sizeof(topicId));
    if (MQTTSNDeserialize_subscribe(&dup, &qos, &mMessageId, &topicId, const_cast<unsigned char*>(aBuffer), aBufferLength) != 1)
    {
        return OT_ERROR_FAILED;
    }
    mDupFlag = static_cast<bool>(dup);
    mQos = static_cast<Qos>(qos);
    SuccessOrExit(error = mTopicName.Set("%.*s", topicId.data.long_.len, topicId.data.long_.name));

exit:
    return error;
}

otError SubackMessage::Serialize(uint8_t* aBuffer, uint8_t aBufferLength, int32_t* aLength) const
{
    int32_t length = MQTTSNSerialize_suback(aBuffer, aBufferLength, static_cast<int>(mQos),
        static_cast<unsigned short>(mTopicId), mMessageId, static_cast<unsigned char>(mReturnCode));
    if (length <= 0)
    {
        return OT_ERROR_FAILED;
    }
    *aLength = length;
    return OT_ERROR_NONE;
}

otError SubackMessage::Deserialize(const uint8_t* aBuffer, int32_t aBufferLength)
{
    unsigned short topicId;
    unsigned char code;
    int qos;
    if (MQTTSNDeserialize_suback(&qos, &topicId, &mMessageId, &code, const_cast<unsigned char*>(aBuffer), aBufferLength))
    {
        return OT_ERROR_FAILED;
    }
    mTopicId = static_cast<TopicId>(topicId);
    mReturnCode = static_cast<ReturnCode>(code);
    mQos = static_cast<Qos>(qos);
    return OT_ERROR_NONE;
}

}

}