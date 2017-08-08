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

using namespace boost::property_tree;
using namespace boost::property_tree::json_parser;
using namespace oauth;
using namespace std;
using namespace http;
OAuth2::OAuth2(string key, string secret, string version, OAuthSecurityMethod method) :
    consumerKey_(key),
    consumerSecret_(secret),
    securityMethod_(method),
    oauthVersion_(version),
    requestFactory_(HttpRequestFactory::createFactory()) {

}

void OAuth2::getToken(string& response, string& token, string& token_type, string& id)
{
  stringstream s;
  ptree pt;

  s << response;

  read_json(s, pt);

  token = pt.get<string>("access_token");
  token_type = pt.get<string>("token_type");

  boost::optional<string> ptOptAccId = pt.get_optional<string>("account_id");
  boost::optional<string> ptOptTeamId = pt.get_optional<string>("team_id");

  if(!ptOptAccId  && ptOptTeamId)
  {
    id = ptOptTeamId.get();
  }
  else if (ptOptAccId  && !ptOptTeamId )
  {
    id = ptOptAccId.get();
  }
  else
  {
    throw OAuthException(MalformedOAuthResponse, "Malformed OAuth response");
  }

}
void OAuth2::getNewTokenFromV1toV2(string& response, string& token)
{
  stringstream s;
  ptree pt;

  s << response;

  read_json(s, pt);

  token = pt.get<string>("oauth2_token");
  //Note need to create an object to convert v1 object to v2
}
void OAuth2::addOAuthHeader(HttpRequest* r, string token) const {
  stringstream ss;

  ss << "Bearer " ;

  string header = ss.str();
  r->addHeader("Authorization", header);
}
void OAuth2::addOAuthBasicHeader(HttpRequest* r, string token, string token_secret) const {
  stringstream ss;

  ss << "Basic "<<  token << ":" << token_secret;

  string header = ss.str();

  r->addHeader("Authorization", header);
}
void OAuth2::addContentTypeHeader(HttpRequest* r) const {
  stringstream ss;

  ss << "application/json" ;

  string header = ss.str();
  r->addHeader("Content-Type", header);
}
void OAuth2::fetchTokenV1toV2(string url, string tokenOauth1, string tokenSecretOauth1)
{
  unique_ptr<HttpRequest> r(requestFactory_->createHttpRequest(url));

  r->setMethod(HttpPostRequest);

  r->addData("oauth1_token_secret", tokenSecretOauth1);
  r->addData("oauth1_token", tokenOauth1);

  if(r->execute() || r->getResponseCode() != 200)
  {
    stringstream ss;
    ss << "Got http error " << r->getResponseCode();
    throw OAuthException(HttpRequestFailed, ss.str());
  }
  string response((char *)r->getResponse(), r->getResponseSize());
  getNewTokenFromV1toV2(response , accessToken_ );
}
void OAuth2::fetchAuthorization(string Response_type)
{//https://www.dropbox.com/1/oauth2/authorize same as below
  string url = "https://www.dropbox.com/oauth2/authorize";
  stringstream ss;
  ss << url;
  ss << "?response_type=" << Response_type << "&";
  ss << "client_id=" << consumerKey_;

  url = ss.str();

  cout<<"\t\t.::Authorization to access Dropbox::."<< endl << endl;
  cout<<"\tTo authorize go to: \n" << url << endl;
  cout << "Enter Authorization code provided after authorization: " << endl;
  getline(std::cin, authorizationCode_);
}

void OAuth2::fetchRequestTokenOauth2(string url)
{
  unique_ptr<HttpRequest> r(requestFactory_->createHttpRequest(url));

  r->setMethod(HttpPostRequest);

  r->addParam("client_secret", consumerSecret_);
  r->addParam("client_id", consumerKey_);
  r->addParam("grant_type", "authorization_code");
  r->addParam("code", authorizationCode_);

  if (r->execute() || r->getResponseCode() != 200) {
    stringstream ss;
    ss << "Got http error " << r->getResponseCode();
    throw OAuthException(HttpRequestFailed, ss.str());
  }

  string response((char *)r->getResponse(), r->getResponseSize());
  getToken(response, accessToken_, tokenType_, accountid_);
}
string OAuth2::getAccessToken() const {
  return accessToken_;
}

void OAuth2::setAccessToken(string token) {
  accessToken_ = token;
}
void OAuth2::addOAuthAccessContentType(HttpRequest* r) const {
  addContentTypeHeader(r);
}
void OAuth2::addOAuthBasicHeader(HttpRequest* r) const {
  addOAuthBasicHeader(r, consumerKey_, consumerSecret_);
}
void OAuth2::addOAuthAccessHeader(HttpRequest* r) const {
  addOAuthHeader(r, accessToken_);
}
/*TODO
Implement errors management for Dropbox API v2
*/
