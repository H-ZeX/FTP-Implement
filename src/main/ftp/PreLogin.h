/**********************************************************************************
*   Copyright © 2018 H-ZeX. All Rights Reserved.
*
*   File Name:    preLogin.h
*   Author:       H-ZeX
*   Create Time:  2018-08-22-22:13:02
*   Describe:
*
**********************************************************************************/
#ifndef __PRE_LOGIN_H__
#define __PRE_LOGIN_H__

#include "src/util/def.h"
#include "src/util/utility.h"
#include <crypt.h>
#include <shadow.h>
#include <string>
using std::string;
/**
 * @return (isValid, (uid_t, gid_t))
 * if isValid == true, the uid_t and gid_t is set
 *
 * MT-safe
 */
UserInfo preLogin(const char *username, const char *passwd, const char *ip = "",
                  const char *port = "");

#endif
