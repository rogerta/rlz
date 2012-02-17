// Copyright 2011 Google Inc. All Rights Reserved.
// Use of this source code is governed by an Apache-style license that can be
// found in the COPYING file.
//
// A library to manage RLZ information for access-points shared
// across different client applications.
//
// All functions return true on success and false on error.
// This implemenation is thread safe.
//
// Each prototype mentions the registry access requirements:
//
// HKLM read:  Will work from any process and at any privilege level on Vista.
// HKCU read:  Calls made from the SYSTEM account must pass the current user's
//             SID as the optional 'sid' param. Can be called from low integrity
//             process on Vista.
// HKCU write: Calls made from the SYSTEM account must pass the current user's
//             SID as the optional 'sid' param. Calls require at least medium
//             integrity on Vista (e.g. Toolbar will need to use their broker)
// HKLM write: Calls must be made from an account with admin rights. No SID
//             need be passed when running as SYSTEM.
// Functions which do not access registry will be marked with "no restrictions".

#ifndef RLZ_WIN_LIB_RLZ_LIB_H_
#define RLZ_WIN_LIB_RLZ_LIB_H_

#include <stdio.h>
#include <string>

#if defined(OS_WIN)
#include "base/memory/scoped_ptr.h"
#include "base/win/registry.h"
#endif

#define RLZ_LIB_API __cdecl

namespace rlz_lib {

class LibMutex;

// An Access Point offers a way to search using Google.
enum AccessPoint {
  NO_ACCESS_POINT = 0,

  // Access points on Windows PCs.
  IE_DEFAULT_SEARCH,  // The IE7+ chrome search box next to the address bar.
  IE_HOME_PAGE,       // Search box on IE 5+ primary home page when Google.
  IETB_SEARCH_BOX,    // IE Toolbar v4+ search box.
  QUICK_SEARCH_BOX,   // Search box brought up by ctrl-ctrl key sequence,
                      // distributed as a part of Google Desktop
  GD_DESKBAND,        // Search box in deskbar when GD in deskbar mode.
  GD_SEARCH_GADGET,   // Search gadget when GD in sidebar mode.
  GD_WEB_SERVER,      // Boxes in web pages shown by local GD web server.
  GD_OUTLOOK,         // Search box installed within outlook by GD.
  CHROME_OMNIBOX,     // Chrome searches through the address bar omnibox.
  CHROME_HOME_PAGE,   // Chrome searches through Google as home page.
  FFTB2_BOX,          // Firefox Toolbar v2 Search Box.
  FFTB3_BOX,          // Firefox Toolbar v3+ Search Box.
  PINYIN_IME_BHO,     // Goopy Input Method Editor BHO (Pinyin).
  IGOOGLE_WEBPAGE,    // Searches on iGoogle through partner deals.

  // Mobile idle screen search for different platforms.
  MOBILE_IDLE_SCREEN_BLACKBERRY,
  MOBILE_IDLE_SCREEN_WINMOB,
  MOBILE_IDLE_SCREEN_SYMBIAN,

  FF_HOME_PAGE,       // Firefox home page when set to Google.
  FF_SEARCH_BOX,      // Firefox search box when set to Google.
  IE_BROWSED_PAGE,    // Search made in IE through user action (no product).
  QSB_WIN_BOX,        // Search box brought up by ctrl+space by default,
                      // distributed by toolbar and separate from the GD QSB
  WEBAPPS_CALENDAR,   // Webapps use of calendar.
  WEBAPPS_DOCS,       // Webapps use of writely.
  WEBAPPS_GMAIL,      // Webapps use of Gmail.

  IETB_LINKDOCTOR,    // Linkdoctor of IE Toolbar
  FFTB_LINKDOCTOR,    // Linkdoctor of FF Toolbar
  IETB7_SEARCH_BOX,   // IE Toolbar search box.
  TB8_SEARCH_BOX,     // IE/FF Toolbar search box.
  CHROME_FRAME,       // Chrome Frame.

  // Partner access points.
  PARTNER_AP_1,
  PARTNER_AP_2,
  PARTNER_AP_3,
  PARTNER_AP_4,
  PARTNER_AP_5,

  // Unclaimed access points - should be used first before creating new APs.
  // Please also make sure you re-name the enum before using an unclaimed value;
  // this acts as a check to ensure we don't have collisions.
  UNDEFINED_AP_H,
  UNDEFINED_AP_I,
  UNDEFINED_AP_J,
  UNDEFINED_AP_K,
  UNDEFINED_AP_L,
  UNDEFINED_AP_M,
  UNDEFINED_AP_N,
  UNDEFINED_AP_O,
  UNDEFINED_AP_P,
  UNDEFINED_AP_Q,
  UNDEFINED_AP_R,
  UNDEFINED_AP_S,
  UNDEFINED_AP_T,
  UNDEFINED_AP_U,
  UNDEFINED_AP_V,
  UNDEFINED_AP_W,
  UNDEFINED_AP_X,
  UNDEFINED_AP_Y,
  UNDEFINED_AP_Z,

  PACK_AP0,
  PACK_AP1,
  PACK_AP2,
  PACK_AP3,
  PACK_AP4,
  PACK_AP5,
  PACK_AP6,
  PACK_AP7,
  PACK_AP8,
  PACK_AP9,
  PACK_AP10,
  PACK_AP11,
  PACK_AP12,
  PACK_AP13,

  // New Access Points should be added here without changing existing enums,
  // (i.e. before LAST_ACCESS_POINT)
  LAST_ACCESS_POINT
};

// A product is an entity which wants to gets credit for setting
// an Access Point.
enum Product {
  IE_TOOLBAR = 1,
  TOOLBAR_NOTIFIER,
  PACK,
  DESKTOP,
  CHROME,
  FF_TOOLBAR,
  QSB_WIN,
  WEBAPPS,
  PINYIN_IME,
  PARTNER
  // New Products should be added here without changing existing enums.
};

// Events that note Product and Access Point modifications.
enum Event {
  INVALID_EVENT = 0,
  INSTALL = 1,    // Access Point added to the system.
  SET_TO_GOOGLE,  // Point set from non-Google provider to Google.
  FIRST_SEARCH,   // First search from point since INSTALL
  REPORT_RLS,     // Report old system "RLS" financial value for this point.
  // New Events should be added here without changing existing enums,
  // before LAST_EVENT.
  ACTIVATE,       // Product being used for a period of time.
  LAST_EVENT
};

// The maximum length of an access points RLZ in WCHARs.
static const int kMaxRlzLength = 64;
// The maximum length of an access points RLZ in WCHARs.
static const int kMaxDccLength = 128;
// The maximum length of a CGI string in WCHARs.
static const int kMaxCgiLength = 2048;
// The maximum length of a ping response we will parse. If the response
// is bigger, please break it up into separate calls.
static const int kMaxPingResponseLength = 0x4000;  // 16K
// The length of the Machine unique ID in WCHARs, excluding the NULL terminator.
static const int kMachineIdLength = 50;


// TODO(thakis): Port these functions.
#if defined(OS_WIN)
// Event storage functions.

// Records an RLZ event.
// Some events can be product-independent (e.g: First search from home page),
// and some can be access point independent (e.g. Pack installed). However,
// product independent events must still include the product which cares about
// that information being reported.
// Access: HKCU write.
bool RLZ_LIB_API RecordProductEvent(Product product, AccessPoint point,
                                    Event event_id, const wchar_t* sid=NULL);

// Get all the events reported by this product as a CGI string to append to
// the daily ping.
// Access: HKCU read.
bool RLZ_LIB_API GetProductEventsAsCgi(Product product, char* unescaped_cgi,
                                       size_t unescaped_cgi_size,
                                       const wchar_t* sid=NULL);

// Clear all reported events and recorded stateful events of this product.
// This should be called on complete uninstallation of the product.
// Access: HKCU write.
bool RLZ_LIB_API ClearAllProductEvents(Product product,
                                       const wchar_t* sid=NULL);

// Clear an event reported by this product. This should be called after a
// successful ping to the RLZ server.
// Access: HKCU write.
bool RLZ_LIB_API ClearProductEvent(Product product, AccessPoint point,
                                   Event event_id, const wchar_t* sid=NULL);

// RLZ storage functions.

// Get the RLZ value of the access point. If the access point is not Google, the
// RLZ will be the empty string and the function will return false.
// Access: HKCU read.
bool RLZ_LIB_API GetAccessPointRlz(AccessPoint point, char* rlz,
                                   size_t rlz_size, const wchar_t* sid=NULL);

// Set the RLZ for the access-point. Fails and asserts if called when the access
// point is not set to Google.
// new_rlz should come from a server-response. Client applications should not
// create their own RLZ values.
// Access: HKCU write.
bool RLZ_LIB_API SetAccessPointRlz(AccessPoint point, const char* new_rlz,
                                   const wchar_t* sid=NULL);



// OEM Deal confirmation storage functions.

// Makes the OEM Deal Confirmation code writable by all users on the machine.
// This should be called before calling SetMachineDealCode from a non-admin
// account.
// Access: HKLM write.
bool RLZ_LIB_API CreateMachineState(void);

// Set the OEM Deal Confirmation Code (DCC). This information is used for RLZ
// initalization.
// Access: HKLM write, or
// HKCU read if rlz_lib::CreateMachineState() has been sucessfully called.
bool RLZ_LIB_API SetMachineDealCode(const char* dcc);

// Get the DCC cgi argument string to append to a daily ping.
// Should be used only by OEM deal trackers. Applications should use the
// GetMachineDealCode method which has an AccessPoint paramter.
// Access: HKLM read.
bool RLZ_LIB_API GetMachineDealCodeAsCgi(char* cgi, size_t cgi_size);

// Get the DCC value stored in registry.
// Should be used only by OEM deal trackers. Applications should use the
// GetMachineDealCode method which has an AccessPoint paramter.
// Access: HKLM read.
bool RLZ_LIB_API GetMachineDealCode(char* dcc, size_t dcc_size);


// Financial Server pinging functions.
// These functions deal with pinging the RLZ financial server and parsing and
// acting upon the response. Clients should SendFinancialPing() to avoid needing
// these functions. However, these functions allow clients to split the various
// parts of the pinging process up as needed (to avoid firewalls, etc).

// Forms the HTTP request to send to the RLZ financial server.
//
// product            : The product to ping for.
// access_points      : The access points this product affects. Array must be
//                      terminated with NO_ACCESS_POINT.
// product_signature  : The signature sent with daily pings (e.g. swg, ietb)
// product_brand      : The brand of the pinging product, if any.
// product_id         : The product-specific installation ID (can be NULL).
// product_lang       : The language for the product (used to determine cohort).
// exclude_machine_id : Whether the Machine ID should be explicitly excluded
//                      based on the products privacy policy.
// request            : The buffer where the function returns the HTTP request.
// request_buffer_size: The size of the request buffer in WCHARs. The buffer
//                      size (kMaxCgiLength+1) is guaranteed to be enough.
// sid                : The user account SID - needed when running as system.
//
// Access: HKCU read.
bool RLZ_LIB_API FormFinancialPingRequest(Product product,
                                          const AccessPoint* access_points,
                                          const char* product_signature,
                                          const char* product_brand,
                                          const char* product_id,
                                          const char* product_lang,
                                          bool exclude_machine_id,
                                          char* request,
                                          size_t request_buffer_size,
                                          const wchar_t* sid=NULL);

// Pings the financial server and returns the HTTP response. This will fail
// if it is too early to ping the server since the last ping.
//
// product              : The product to ping for.
// request              : The HTTP request (for example, returned by
//                        FormFinancialPingRequest).
// response             : The buffer in which the HTTP response is returned.
// response_buffer_size : The size of the response buffer in WCHARs. The buffer
//                        size (kMaxPingResponseLength+1) is enough for all
//                        legitimate server responses (any response that is
//                        bigger should be considered the same way as a general
//                        network problem).
// sid                  : The user account SID - needed when running as system.
//
// Access: HKCU read.
bool RLZ_LIB_API PingFinancialServer(Product product,
                                     const char* request,
                                     char* response,
                                     size_t response_buffer_size,
                                     const wchar_t* sid=NULL);

// Parses the responses from the financial server and updates product state
// and access point RLZ's in registry.
// Access: HKCU write.
bool RLZ_LIB_API ParseFinancialPingResponse(Product product,
                                            const char* response,
                                            const wchar_t* sid=NULL);

// Complex helpers built on top of other functions.

// Copies the events associated with the product and the RLZ's for each access
// point in access_points into cgi. This string can be directly appended
// to a ping (will need an & if not first paramter).
// access_points must be an array of AccessPoints terminated with
// NO_ACCESS_POINT.
// Access: HKCU read.
bool RLZ_LIB_API GetPingParams(Product product,
                               const AccessPoint* access_points,
                               char* unescaped_cgi, size_t unescaped_cgi_size,
                               const wchar_t* sid=NULL);

// Parses RLZ related ping response information from the server.
// Updates stored RLZ values and clears stored events accordingly.
// Access: HKCU write.
bool RLZ_LIB_API ParsePingResponse(Product product, const char* response,
                                   const wchar_t* sid=NULL);

// Checks if a ping response is valid - ie. it has a checksum line which
// is the CRC-32 checksum of the message uptil the checksum. If
// checksum_idx is not NULL, it will get the index of the checksum, i.e. -
// the effective end of the message.
// Access: No restrictions.
bool RLZ_LIB_API IsPingResponseValid(const char* response,
                                     int* checksum_idx);

// Parses a ping response, checks if it is valid and sets the machine DCC
// from the response. The ping must also contain the current DCC value in
// order to be considered valid.
// Access: HKLM write;
//         HKCU write if CreateMachineState() has been successfully called.
bool RLZ_LIB_API SetMachineDealCodeFromPingResponse(const char* response);

// Send the ping with RLZs and events to the PSO server.
// This ping method should be called daily. (More frequent calls will fail).
// Also, if there are no events, the call will succeed only once a week.
//
// product            : The product to ping for.
// access_points      : The access points this product affects. Array must be
//                      terminated with NO_ACCESS_POINT.
// product_signature  : The signature sent with daily pings (e.g. swg, ietb)
// product_brand      : The brand of the pinging product, if any.
// product_id         : The product-specific installation ID (can be NULL).
// product_lang       : The language for the product (used to determine cohort).
// exclude_machine_id : Whether the Machine ID should be explicitly excluded
//                      based on the products privacy policy.
// sid                : The user account SID - needed when running as system.
//
// Returns true on successful ping and response, false otherwise.
// Access: HKCU write.
bool RLZ_LIB_API SendFinancialPing(Product product,
                                   const AccessPoint* access_points,
                                   const char* product_signature,
                                   const char* product_brand,
                                   const char* product_id,
                                   const char* product_lang,
                                   bool exclude_machine_id,
                                   const wchar_t* sid=NULL);

// An alternate implementations of SendFinancialPing with the same behavior,
// except the caller can optionally choose to skip the timing check.
bool RLZ_LIB_API SendFinancialPing(Product product,
                                   const AccessPoint* access_points,
                                   const char* product_signature,
                                   const char* product_brand,
                                   const char* product_id,
                                   const char* product_lang,
                                   bool exclude_machine_id,
                                   const wchar_t* sid,
                                   const bool skip_time_check);



// Clears all product-specifc state from the RLZ registry.
// Should be called during product uninstallation.
// This removes outstanding product events, product financial ping times,
// the product RLS argument (if any), and any RLZ's for access points being
// uninstalled with the product.
// access_points is an array terminated with NO_ACCESS_POINT.
// IMPORTANT: These are the access_points the product is removing as part
// of the uninstallation, not necessarily all the access points passed to
// SendFinancialPing() and GetPingParams().
// access_points can be NULL if no points are being uninstalled.
// No return value - this is best effort. Will assert in debug mode on
// failed attempts.
// Access: HKCU write.
void RLZ_LIB_API ClearProductState(Product product,
                                   const AccessPoint* access_points,
                                   const wchar_t* sid=NULL);

// Gets the unique ID for the machine used for RLZ tracking purposes. This ID
// is derived from the Windows machine SID, and is the string representation of
// a 20 byte hash + a 1 byte checksum.
// Included in financial pings with events, unless explicitly forbidden by the
// calling application.
// Access: HKLM read.
bool GetMachineId(char* buffer, int buffer_size);


// Segment RLZ persistence based on branding information.
// The RLZ library uses the Windows registry to save persistent information.
// All information for a given product is persisted under keys with the either
// product's name or its access point's name.  This assumes that only
// one instance of the product is installed on the machine, and that only one
// product brand is associated with it.
//
// In some cases, a given product may be using supplementary brands.  The RLZ
// information must be kept separately for each of these brands.  To achieve
// this segmentation, scope all RLZ library calls that deal with supplementary
// brands within the lifetime of an rlz_lib::ProductBranding instance.
//
// For example, to record events for a supplementary brand, do the following:
//
//  {
//    rlz_lib::SupplementaryBranding branding("AAAA");
//    // This call to RecordProductEvent is scoped to the AAAA brand.
//    rlz_lib::RecordProductEvent(rlz_lib::DESKTOP, rlz_lib::GD_DESKBAND,
//                                rlz_lib::INSTALL);
//  }
//
//  // This call to RecordProductEvent is not scoped to any supplementary brand.
//  rlz_lib::RecordProductEvent(rlz_lib::DESKTOP, rlz_lib::GD_DESKBAND,
//                              rlz_lib::INSTALL);
//
// In particular, this affects the recording of stateful events and the sending
// of financial pings.  In the former case, a stateful event recorded while
// scoped to a supplementary brand will be recorded again when scoped to a
// different supplementary brand (or not scoped at all).  In the latter case,
// the time skip check is specific to each supplementary brand.
class SupplementaryBranding {
 public:
  SupplementaryBranding(const wchar_t* brand);
  ~SupplementaryBranding();

  static const std::wstring& GetBrand() { return brand_; }

  static void AppendBrandToString(std::wstring* str);

 private:
  scoped_ptr<LibMutex> lock_;

  static std::wstring brand_;
};

// Initialize temporary HKLM/HKCU registry hives used for testing.
// Testing RLZ requires reading and writing to the Windows registry.  To keep
// the tests isolated from the machine's state, as well as to prevent the tests
// from causing side effects in the registry, HKCU and HKLM are overridden for
// the duration of the tests. RLZ tests don't expect the HKCU and KHLM hives to
// be empty though, and this function initializes the minimum value needed so
// that the test will run successfully.
//
// The two arguments to this function should be the keys that will represent
// the HKLM and HKCU registry hives during the tests.  This function should be
// called *before* the hives are overridden.
void InitializeTempHivesForTesting(const base::win::RegKey& temp_hklm_key,
                                   const base::win::RegKey& temp_hkcu_key);
#endif  // defined(OS_WIN)

}  // namespace rlz_lib

#endif  // RLZ_WIN_LIB_RLZ_LIB_H_
