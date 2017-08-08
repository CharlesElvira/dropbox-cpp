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

#include "DropboxAccountInfo.h"
#include "DropboxException.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <sstream>

using namespace dropbox;
using namespace std;
using namespace boost::property_tree;
using namespace boost::property_tree::json_parser;

void DropboxAccountInfo::readFromJson(DropboxAccountInfo* info, string& json) {
  try {
    stringstream ss;
    ss << json;

    ptree pt;
    read_json(ss, pt);
//NOTE Need to test boost json_parser from string when embedded
    //info->DropboxNameInfo_.givenName_ = pt.get_optional<string>("profile_photo_url");
    //info->DropboxNameInfo_.surname_ = pt.get_optional<string>("profile_photo_url");
    //info->DropboxNameInfo_.familiarName_ = pt.get_optional<string>("profile_photo_url");
    //info->DropboxNameInfo_.displayName_ = pt.get_optional<string>("profile_photo_url");
    //info->DropboxNameInfo_.abbreviatedName_ = pt.get_optional<string>("profile_photo_url");
    info->isTeammate_ = pt.get<bool>("is_teammate");
    info->disabled_ = pt.get<bool>("disabled");
    info->emailVerified_ = pt.get<bool>("email_verified");
    info->accountId_ = pt.get<string>("account_id");
    info->email_ = pt.get<string>("email");
    //info->profilePhotoUrl_ = pt.get_optional<string>("profile_photo_url");
    //info->teamMemberId_ = pt.get_optional<string>("team_member_id");


  } catch (exception& e) {
    throw DropboxException(MALFORMED_RESPONSE, e.what());
  }
}

DropboxAccountInfo::DropboxAccountInfo(string& json) {
  readFromJson(this, json);
}

void DropboxAccountInfo::readJson(string& json) {
  readFromJson(this, json);
}

string DropboxAccountInfo::getDisplayName() const {
  return DropboxNameInfo_.displayName_;
}

string DropboxAccountInfo::getEmail() const {
  return email_;
}
