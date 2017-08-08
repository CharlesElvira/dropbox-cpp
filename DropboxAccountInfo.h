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

#ifndef __DROPBOX_ACCOUNT_INFO_H__
#define __DROPBOX_ACCOUNT_INFO_H__

#include <string>

#include <sys/types.h>
namespace dropbox {

struct DropboxName
{
  std::string         givenName_;
  std::string         surname_;
  std::string         familiarName_;
  std::string         displayName_;
  std::string         abbreviatedName_;
};

class DropboxAccountInfo {
public:
  DropboxAccountInfo() { }
  DropboxAccountInfo(std::string& json);

  void            readJson(std::string&);

  std::string         getEmail() const;


  std::string         getName() const;
  std::string         getGivenName() const;
  std::string         getSurname() const;
  std::string         getFamiliarName() const;
  std::string         getDisplayName() const;
  std::string         getAbbreviatedName() const;
  std::string         getAccountId() const;
  std::string         getProfilePhotoUrl() const;

  bool                isEmailVerified() const;
  bool                isDisabled() const;
  bool                isTeammate() const;

  bool                getEmailVerified() const;

private:
  static void     readFromJson(DropboxAccountInfo*, std::string& json);

  DropboxName           DropboxNameInfo_;
  std::string           email_;

  std::string           accountId_;
  std::string           *teamMemberId_;
  std::string           *profilePhotoUrl_;

  bool                  emailVerified_;
  bool                  disabled_;
  bool                  isTeammate_;

};
}
#endif
/*TODO
Implement account_id verification and Error
Implement Error for no_accounr and Other
*/
