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

#include "HttpRequestFactory.h"
#include "HttpRequest.h"
#include "OAuthException.h"
#include "OAuth2.h"

#include <sstream>

using namespace oauth2;
using namespace std;
using namespace http;
OAuth2::OAuth2(string key, string version, OAuthSecurityMethod method) :
    consumerKey_(key),
    consumerSecret_(secret),
    securityMethod_(method),
    oauthVersion_(version),
    requestFactory_(HttpRequestFactory::createFactory()) {
    fetchAuthorization("code", key);
}
OAuth2::OAuth2(string key,
  string secret, string version, OAuthSecurityMethod method) :
    consumerKey_(key),
    consumerSecret_(secret),
    securityMethod_(method),
    oauthVersion_(version),
    requestFactory_(HttpRequestFactory::createFactory()) {
}

void OAuth2::splitParams(string& response, map<string, string>& params) {
  size_t idx = 0;
  size_t len  = response.size();

  do {
    size_t ampPos = response.find("&", idx);
    if (ampPos == string::npos) {
      ampPos = len;
    }

    string paramStr = response.substr(idx, ampPos - idx);
    size_t eqPos = paramStr.find("=");
    if (eqPos == string::npos) {
      throw OAuthException(MalformedOAuthResponse, "Malformed OAuth response");
    }

    string key = paramStr.substr(0, eqPos);
    string value = paramStr.substr(eqPos + 1);

    params[key] = value;

    idx = ampPos + 1;
  } while(idx < len);
}

void OAuth2::getTokenAndSecret(string& response, string& token, string& secret) {
  map<string, string> params;

  splitParams(response, params);

  auto i = params.find("oauth_token");
  if (i == params.end()) {
    throw OAuthException(MalformedOAuthResponse, "Malformed OAuth response");
  }

  token = i->second;

  i = params.find("oauth_token_secret");
  if (i == params.end()) {
    throw OAuthException(MalformedOAuthResponse, "Malformed OAuth response");
  }

  secret = i->second;
}
void OAuth2::addOAuthHeaderOauth2(HttpRequest* r, string token, string secret) const {
  stringstream ss;

  ss << "Bearer <\"" << token ;

  string header = ss.str();
  r->addHeader("Authorization", header);
}

void OAuth2::addOAuthHeader(HttpRequest* r, string token, string secret) const {
  stringstream ss;

  ss << "OAuth oauth_version=\"" << oauthVersion_ << "\", ";
  ss << "oauth_consumer_key=\"" << consumerKey_ << "\", ";

  if (token.compare("")) {
    ss << "oauth_token=\"" << token << "\", ";
  }

  string signatureMethod;
  string signature;
  switch (securityMethod_) {
    case OAuthSecPlaintext:
      {
        signatureMethod="PLAINTEXT";
        stringstream sst;
        sst << consumerSecret_ << "&" << secret;
        signature = sst.str();
      }
      break;
    default:
      throw OAuthException(UnsupportedMethod,
        "Only Plaintext signature supported");
  }

  ss << "oauth_signature_method=\"" << signatureMethod << "\", ";
  ss << "oauth_signature=\"" << signature << "\"";

  string header = ss.str();
  r->addHeader("Authorization", header);
}
void OAuth2::fetchAuthorization(string Response_type, string key)
{
  string url = "https://www.dropbox.com/oauth2/authorize";
  stringstream ss;
  ss << "?response_type=\"" << Response_type << "\", ";
  ss << "client_id=\"" << key;

  url.push_back(ss.str());

  cout<<".::Authorization to access Dropbox::."<<endl;
  cout<<"Go to " << url << " to authorize" << endl;
  cout << "Enter Authorization code provided after authorization:" << endl;
  authorizationCode_ << cin;
}
void OAuth2::fetchRequestTokenOauth2(string url)
{
  unique_ptr<HttpRequest> r(requestFactory_->createHttpRequest(url));

  r->setMethod(HttpPostRequest);

  r->addParam("code", authorizationCode_);
  r->addParam("grant_type", "authorization_code");
  r->addParam("client_id", token);
  r->addParam("client_secret", secret);

  addOAuthHeader(r.get(), "", "");

  if (r->execute() || r->getResponseCode() != 200) {
    stringstream ss;
    ss << "Got http error " << r->getResponseCode();

    throw OAuthException(HttpRequestFailed, ss.str());
  }

  string response((char *)r->getResponse(), r->getResponseSize());
  getTokenAndSecret(response, requestToken_, requestSecret_);
}
void OAuth2::fetchRequestToken(string url) {
  unique_ptr<HttpRequest> r(requestFactory_->createHttpRequest(url));

  r->setMethod(HttpPostRequest);

  addOAuthHeader(r.get(), "", "");

  if (r->execute() || r->getResponseCode() != 200) {
    stringstream ss;
    ss << "Got http error " << r->getResponseCode();

    throw OAuthException(HttpRequestFailed, ss.str());
  }

  string response((char *)r->getResponse(), r->getResponseSize());
  getTokenAndSecret(response, requestToken_, requestSecret_);
}

void OAuth2::fetchAccessToken(string url) {
  unique_ptr<HttpRequest> r(requestFactory_->createHttpRequest(url));

  r->setMethod(HttpPostRequest);

  addOAuthHeader(r.get(), requestToken_, requestSecret_);

  if (r->execute() || r->getResponseCode() != 200) {
    stringstream ss;
    ss << "Got http error " << r->getResponseCode();

    throw OAuthException(HttpRequestFailed, ss.str());
  }

  string response((char *)r->getResponse(), r->getResponseSize());
  getTokenAndSecret(response, accessToken_, accessSecret_);
}

string OAuth2::getRequestToken() const {
  return requestToken_;
}

string OAuth2::getRequestTokenSecret() const {
  return requestSecret_;
}

string OAuth2::getAccessToken() const {
  return accessToken_;
}

string OAuth2::getAccessTokenSecret() const {
  return accessSecret_;
}

void OAuth2::setAccessToken(string token) {
  accessToken_ = token;
}

void OAuth2::setAccessTokenSecret(string secret) {
  accessSecret_ = secret;
}

void OAuth2::addOAuthAccessHeader(HttpRequest* r) const {
  addOAuthHeader(r, accessToken_, accessSecret_);
}
