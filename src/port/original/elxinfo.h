#pragma once

/*
 * MERGEELX emits the table body only when elxdefs.h has defined elkAppMac.
 * eldlg.c intentionally includes elxinfo.h without elxdefs.h, so the
 * original generated header contributes no declarations in this unit.
 */
