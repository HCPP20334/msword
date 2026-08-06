#pragma once

/*
 * The checked-in archive contains the Dialog Editor .des sources and the
 * original MERGEELX source, but not the Dialog Editor executable which made
 * MERGEELX's intermediate .elx files.  Keep the EL interpreter compileable
 * at that documented boundary.  The 560-entry application ELK range comes
 * from dlg/syncelx.txt and preserves the original user-keyword offset.
 */
#define ieldiIDDUsrDlg 0
#define elkAppMac 560
#define ibstElkAppMac 1
