/*
* Copyright (c) 2013 Rahul Iyer
* All rights reserved.
*
* Redistribution and use in source and binary forms are permitted provided that
* the above copyright notice and this paragraph are duplicated in all such forms
* and that any documentation, advertising materials, and other materials related
* to such distribution and use acknowledge that the software was developed by
* Rahul Iyer.  The name of Rahul Iyer may not be used to endorse or promote
* products derived from this software without specific prior written permission.
* THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "DropboxApi2.h"

#include "util/HttpRequest.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <sstream>
#include <cassert>

using namespace dropbox;
using namespace oauth;
using namespace http;
using namespace std;

using namespace boost::property_tree;
using namespace boost::property_tree::json_parser;

DropboxApi2::DropboxApi2(string appKey, string appSecret) {
  httpFactory_ = HttpRequestFactory::createFactory();

  lock_guard<mutex> g(stateLock_);
  oauth_.reset(new OAuth2(appKey, appSecret));
  root_ = DROPBOX_ROOT;
}

DropboxApi2::DropboxApi2(string appKey,
    string appSecret,
    string accessToken) {
  httpFactory_ = HttpRequestFactory::createFactory();

  lock_guard<mutex> g(stateLock_);
  oauth_.reset(new OAuth2(appKey, appSecret));
  oauth_->setAccessToken(accessToken);
}
void DropboxApi2::authenticate() {
  lock_guard<mutex> g(stateLock_);

  oauth_->fetchAuthorization("code");
//https://www.dropbox.com/oauth2/authorize same as below
  oauth_->fetchRequestTokenOauth2("https://api.dropboxapi.com/1/oauth2/token");
}

void DropboxApi2::setAccessToken(string token) {
  lock_guard<mutex> g(stateLock_);
  oauth_->setAccessToken(token);
}

string DropboxApi2::getAccessToken() {
  lock_guard<mutex> g(stateLock_);
  return oauth_->getAccessToken();
}

void DropboxApi2::setRoot(const string root) {
  lock_guard<mutex> g(stateLock_);
  root_ = root;
}

DropboxErrorCode DropboxApi2::execute(shared_ptr<HttpRequest> r) {
  int ret;

  {
    lock_guard<mutex> g(stateLock_);
    oauth_->addOAuthAccessHeader(r.get());
  }

  if ((ret = r->execute())) {
    stringstream ss;
    ss << "Curl error (code = " << ret << ")";

    throw DropboxException(CURL_ERROR, ss.str());
  }

  return (DropboxErrorCode)r->getResponseCode();
}

DropboxErrorCode DropboxApi2::getAccountInfo(DropboxAccountInfo& info) {
  shared_ptr<HttpRequest> r(
    httpFactory_->createHttpRequest("https://api.dropbox.com/1/account/info"));

  DropboxErrorCode code = execute(r);

  if (code == SUCCESS) {
    string response((char *)r->getResponse(), r->getResponseSize());
    info.readJson(response);
  }

  return code;
}

DropboxErrorCode DropboxApi2::getFileMetadata(DropboxMetadataRequest& req,
    DropboxMetadataResponse& res) {
  stringstream ss;
  ss << "https://api.dropbox.com/1/metadata/" << root_ << "/" << req.path();

  shared_ptr<HttpRequest> r(httpFactory_->createHttpRequest(ss.str()));

  r->setMethod(HttpGetRequest);

  r->addIntegerParam("file_limit", req.getLimit());

  if (req.getHash().compare("")) {
    r->addParam("hash", req.getHash());
  }

  if (req.includeChildren()) {
    r->addParam("list", "true");
  } else {
    r->addParam("list", "false");
  }

  if (req.includeDeleted()) {
    r->addParam("include_deleted", "true");
  } else {
    r->addParam("include_deleted", "false");
  }

  if (req.getRev().compare("")) {
    r->addParam("rev", req.getRev());
  }

  DropboxErrorCode code = execute(r);
  if (code != SUCCESS) {
    return code;
  }

  string response((char *)r->getResponse(), r->getResponseSize());
  res.readJson(response);

  return code;
}

DropboxErrorCode DropboxApi2::getRevisions(string path,
    size_t numRevisions, DropboxRevisions& revs) {
  stringstream ss;

  ss << "https://api.dropbox.com/1/revisions/" << root_ << "/" << path;

  shared_ptr<HttpRequest> r(httpFactory_->createHttpRequest(ss.str()));

  r->setMethod(HttpGetRequest);

  if (numRevisions) {
    r->addIntegerParam("rev_limit", numRevisions);
  }

  DropboxErrorCode code = execute(r);
  if (code != SUCCESS) {
    return code;
  }

  string response((char *)r->getResponse(), r->getResponseSize());
  revs.readFromJson(response);

  return code;
}

DropboxErrorCode DropboxApi2::restoreFile(string path,
    string rev, DropboxMetadata& m) {
  stringstream ss;

  ss << "https://api.dropbox.com/1/restore/" << root_ << "/" << path;

  shared_ptr<HttpRequest> r(httpFactory_->createHttpRequest(ss.str()));

  r->setMethod(HttpPostRequest);
  r->addParam("rev", rev);

  DropboxErrorCode code = execute(r);
  if (code != SUCCESS) {
    return code;
  }

  string response((char *)r->getResponse(), r->getResponseSize());

  stringstream s;
  s << response;

  ptree pt;
  read_json(s, pt);

  DropboxMetadata::readFromJson(pt, m);

  return code;
}

DropboxErrorCode DropboxApi2::deleteFile(string path, DropboxMetadata& m) {
  shared_ptr<HttpRequest> r(httpFactory_->createHttpRequest(
    "https://api.dropbox.com/1/fileops/delete"));

  r->addParam("root", root_);
  r->addParam("path", path);

  DropboxErrorCode code = execute(r);
  if (code != SUCCESS) {
    return code;
  }

  string response((char *)r->getResponse(), r->getResponseSize());

  stringstream s;
  s << response;

  ptree pt;
  read_json(s, pt);

  DropboxMetadata::readFromJson(pt, m);

  return code;
}

DropboxErrorCode DropboxApi2::copyOrMove(const string from,
    const string to,
    const string op,
    DropboxMetadata& m) {
  stringstream ss;
  ss << "https://api.dropbox.com/1/fileops/" << op;

  shared_ptr<HttpRequest> r(httpFactory_->createHttpRequest(ss.str()));

  r->addParam("root", root_);
  r->addParam("from_path", from);
  r->addParam("to_path", to);

  DropboxErrorCode code = execute(r);
  if (code != SUCCESS) {
    return code;
  }

  string response((char *)r->getResponse(), r->getResponseSize());

  stringstream s;
  s << response;

  ptree pt;
  read_json(s, pt);

  DropboxMetadata::readFromJson(pt, m);

  return code;
}

DropboxErrorCode DropboxApi2::copyFile(string from,
    string to,
    DropboxMetadata& m) {
  return copyOrMove(from, to, "copy", m);
}

DropboxErrorCode DropboxApi2::moveFile(string from,
    string to,
    DropboxMetadata& m) {
  return copyOrMove(from, to, "move", m);
}

DropboxErrorCode DropboxApi2::createFolder(const string path,
    DropboxMetadata& m) {
  shared_ptr<HttpRequest> r(httpFactory_->createHttpRequest(
    "https://api.dropbox.com/1/fileops/create_folder"));

  r->addParam("root", root_);
  r->addParam("path", path);

  DropboxErrorCode code = execute(r);
  if (code != SUCCESS) {
    return code;
  }

  string response((char *)r->getResponse(), r->getResponseSize());

  stringstream s;
  s << response;

  ptree pt;
  read_json(s, pt);

  DropboxMetadata::readFromJson(pt, m);

  return code;
}

DropboxErrorCode DropboxApi2::getFile(DropboxGetFileRequest& req,
    DropboxGetFileResponse& res) {
  stringstream ss;
  ss << "https://api-content.dropbox.com/1/files/" << root_ << "/"
    << req.getPath();

  shared_ptr<HttpRequest> r(httpFactory_->createHttpRequest(ss.str()));
  if (req.getRev().compare("")) {
    r->addParam("rev", req.getRev());
  }

  if (req.hasRange()) {
    r->addRange(req.getOffset(), req.getOffset() + req.getLength() - 1);
  }

  DropboxErrorCode code = execute(r);
  if (code != SUCCESS && code != PARTIAL_CONTENT) {
    return code;
  }

  res.setData(r->getResponse(), r->getResponseSize());

  map<string, string> respHeaders = r->getResponseHeaders();
  res.setMetadata(respHeaders["x-dropbox-metadata"]);

  return code;
}

DropboxErrorCode DropboxApi2::uploadFile(const DropboxUploadFileRequest& req,
    DropboxMetadata& m) {
  stringstream ss;
  ss << "https://api-content.dropbox.com/1/files_put/" << root_ << "/"
    << req.getPath();

  shared_ptr<HttpRequest> r(httpFactory_->createHttpRequest(ss.str()));
  r->setMethod(HttpPutRequest);

  if (req.shouldOverwrite()) {
    r->addParam("overwrite", "true");
  } else {
    r->addParam("overwrite", "false");
  }

  if (req.getParentRev().compare("")) {
    r->addParam("parent_rev", req.getParentRev());
  }

  assert(req.getUploadData());

  r->setRequestData(req.getUploadData(), req.getUploadDataSize());

  DropboxErrorCode code = execute(r);

  if (code != SUCCESS) {
    return code;
  }

  string response((char *)r->getResponse(), r->getResponseSize());

  stringstream s;
  s << response;

  ptree pt;
  read_json(s, pt);

  DropboxMetadata::readFromJson(pt, m);

  return code;
}

DropboxErrorCode DropboxApi2::uploadLargeFile(
    const DropboxUploadLargeFileRequest& req,
    DropboxMetadata& m) {
  string uploadId = "";
  size_t offset = req.getOffset();
  size_t size = 0;
  unique_ptr<uint8_t, void(*)(void*)> data(
    (uint8_t*)malloc(req.getChunkSize()), free);

  if (!data.get()) {
    throw std::bad_alloc();
  }

  do {
    shared_ptr<HttpRequest> r(httpFactory_->createHttpRequest(
      "https://api-content.dropbox.com/1/chunked_upload"));
    r->setMethod(HttpPutRequest);
    r->addIntegerParam("offset", offset);

    if (uploadId.compare("")) {
      r->addParam("upload_id", uploadId);
    }

    size = req.getData(data.get(), offset, req.getChunkSize());
    if (!size) {
      continue;
    }

    if (size < 0) {
      throw DropboxException(IO_ERROR, "Error receiving file data");
    }

    r->setRequestData(data.get(), size);

    DropboxErrorCode code = execute(r);
    if (code != SUCCESS) {
      return code;
    }

    string response((char *)r->getResponse(), r->getResponseSize());

    DropboxUploadLargeFileResponse res =
      DropboxUploadLargeFileResponse::readFromJson(response);
    uploadId = res.getUploadId();
    offset = res.getOffset();
  } while (size != 0);

  stringstream ss;
  ss << "https://api-content.dropbox.com/1/commit_chunked_upload/" << root_
    << "/" << req.getPath();

  shared_ptr<HttpRequest> r(httpFactory_->createHttpRequest(ss.str()));
  r->setMethod(HttpPostRequest);

  if (req.shouldOverwrite()) {
    r->addParam("overwrite", "true");
  } else {
    r->addParam("overwrite", "false");
  }

  if (req.getParentRev().compare("")) {
    r->addParam("parent_rev", req.getParentRev());
  }

  r->addParam("upload_id", uploadId);

  DropboxErrorCode code = execute(r);
  if (code != SUCCESS) {
    return code;
  }

  string response((char *)r->getResponse(), r->getResponseSize());

  stringstream s;
  s << response;

  ptree pt;
  read_json(s, pt);

  DropboxMetadata::readFromJson(pt, m);

  return code;
}

DropboxErrorCode DropboxApi2::search(const DropboxSearchRequest& req,
    DropboxSearchResult& res) {
  stringstream ss;
  ss << "https://api.dropbox.com/1/search/" << root_ << "/"
    << req.getSearchPath();

  shared_ptr<HttpRequest> r(httpFactory_->createHttpRequest(ss.str()));

  r->addParam("query", req.getSearchQuery());
  r->addIntegerParam("file_limit", req.getResultLimit());

  if (req.shouldIncludeDeleted()) {
    r->addParam("include_deleted", "true");
  } else {
    r->addParam("include_deleted", "false");
  }

  DropboxErrorCode code = execute(r);

  if (code != SUCCESS) {
    return code;
  }

  string response((char *)r->getResponse(), r->getResponseSize());
  res = DropboxSearchResult::readFromJson(response);

  return code;
}
