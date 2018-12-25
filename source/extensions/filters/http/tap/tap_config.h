#pragma once

#include "envoy/service/tap/v2alpha/common.pb.h"

#include "common/protobuf/protobuf.h"

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace TapFilter {

/**
 * fixfix
 */
class HttpPerRequestTapper {
public:
  virtual ~HttpPerRequestTapper() = default;

  /**
   * fixfix
   */
  virtual void onRequestHeaders(const Http::HeaderMap& headers) PURE;

  /**
   * fixfix
   */
  virtual void onResponseHeaders(const Http::HeaderMap& headers) PURE;

  /**
   *
   */
  virtual void onLog(const Http::HeaderMap* request_headers,
                     const Http::HeaderMap* response_headers) PURE;
};

typedef std::unique_ptr<HttpPerRequestTapper> HttpPerRequestTapperPtr;

/**
 * fixfix
 */
class TapSink {
public:
  virtual ~TapSink() = default;

  /**
   * fixfix
   */
  virtual void submitBufferedTrace(std::shared_ptr<Protobuf::Message> trace) PURE;
};

/**
 * fixfix
 */
class HttpTapConfig {
public:
  virtual ~HttpTapConfig() = default;

  /**
   * fixfix
   */
  virtual HttpPerRequestTapperPtr newPerRequestTapper() PURE;
};

typedef std::shared_ptr<HttpTapConfig> HttpTapConfigSharedPtr;

/**
 * fixfix
 */
class TapConfigFactory {
public:
  virtual ~TapConfigFactory() = default;

  /**
   * fixfix
   */
  virtual HttpTapConfigSharedPtr
  createHttpConfigFromProto(envoy::service::tap::v2alpha::TapConfig&& proto_config,
                            TapSink& admin_streamer) PURE;
};

typedef std::unique_ptr<TapConfigFactory> TapConfigFactoryPtr;

} // namespace TapFilter
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy
