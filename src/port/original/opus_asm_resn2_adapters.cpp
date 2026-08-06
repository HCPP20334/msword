#include "opus_x64_layout.h"

/*
 * AMD64 translations and entry adapters for the remaining resident/resn2
 * frame-access and coordinate routines.  The opaque entry types deliberately
 * keep Microsoft C in control of the historical structure layouts.
 */

struct DR;
struct DRF;

struct OpusPoint {
    int x;
    int y;
};

struct OpusRect {
    int left;
    int top;
    int right;
    int bottom;
};

struct OpusDrc {
    int x;
    int y;
    int dx;
    int dy;
};

extern "C" {

DR* C_PdrFetch(void** hpldr, int idr, DRF* pdrf);
DR* C_PdrFetchAndFree(void** hpldr, int idr, DRF* pdrf);
DR* C_PdrFreeAndFetch(void** hpldr, int idr, DRF* pdrf);
void C_FreePdrf(DRF* pdrf);
OpusPoint C_PtOrigin(void** hpldr, int idr);
extern void** vhwwdOrigin;

int C_FormatLine(int ww, int doc, long cp);
int C_FormatDrLine(int ww, long cp, DR* pdr);
int C_FormatLineDxa(int ww, int doc, long cp, int dxa);
int C_ClFormatLines(void* plbs, long cp_lim, int dyl_fill, int dyl_max,
                    int cl_lim, int dxl, int ref_line, int stop_at_page_break);

int C_FInsertRgch(int doc, long cp, char* text, int count, void* chp,
                  void* pap);
long C_FcAppendRgchToFn(int fn, char* text, int count);
void C_ScanFnForBytes(int fn, char* text, unsigned count, int write);
int C_CbGrpprlProp(int normalized, char* output, int output_max,
                   void* properties, void* base_properties, int word_count,
                   void* property_map);
int C_IfldFromDocCp(int doc, long cp, int match);

DR* N_PdrFetch(void** hpldr, int idr, DRF* pdrf) {
    return C_PdrFetch(hpldr, idr, pdrf);
}

DR* N_PdrFetchAndFree(void** hpldr, int idr, DRF* pdrf) {
    return C_PdrFetchAndFree(hpldr, idr, pdrf);
}

DR* N_PdrFreeAndFetch(void** hpldr, int idr, DRF* pdrf) {
    return C_PdrFreeAndFetch(hpldr, idr, pdrf);
}

void N_FreePdrf(DRF* pdrf) {
    C_FreePdrf(pdrf);
}

OpusPoint N_PtOrigin(void** hpldr, int idr) {
    return C_PtOrigin(hpldr, idr);
}

static OpusPoint WindowOffset(const OpusPoint origin) {
    int page_x;
    int page_y;
    int window_x;
    int window_y;
    OpusWwdCoordinateOffsets(*vhwwdOrigin, &page_x, &page_y, &window_x,
                             &window_y);
    return {origin.x + page_x + window_x, origin.y + page_y + window_y};
}

static void TranslateRect(const OpusRect* source, OpusRect* destination,
                          const int dx, const int dy) {
    destination->left = source->left + dx;
    destination->top = source->top + dy;
    destination->right = source->right + dx;
    destination->bottom = source->bottom + dy;
}

void RclToRcw(void** hpldr, const OpusRect* rcl, OpusRect* rcw) {
    const OpusPoint offset = WindowOffset(C_PtOrigin(hpldr, -1));
    TranslateRect(rcl, rcw, offset.x, offset.y);
}

void RcwToRcl(void** hpldr, const OpusRect* rcw, OpusRect* rcl) {
    const OpusPoint offset = WindowOffset(C_PtOrigin(hpldr, -1));
    TranslateRect(rcw, rcl, -offset.x, -offset.y);
}

void RcwToRcp(void** hpldr, int idr, const OpusRect* rcw, OpusRect* rcp) {
    const OpusPoint offset = WindowOffset(C_PtOrigin(hpldr, idr));
    TranslateRect(rcw, rcp, -offset.x, -offset.y);
}

void RcpToRcw(void** hpldr, int idr, const OpusRect* rcp, OpusRect* rcw) {
    const OpusPoint offset = WindowOffset(C_PtOrigin(hpldr, idr));
    TranslateRect(rcp, rcw, offset.x, offset.y);
}

void RceToRcw(void* pwwd, const OpusRect* rce, OpusRect* rcw) {
    int page_x;
    int page_y;
    int window_x;
    int window_y;
    OpusWwdCoordinateOffsets(pwwd, &page_x, &page_y, &window_x, &window_y);
    TranslateRect(rce, rcw, window_x, window_y);
}

void DrclToRcw(void** hpldr, const OpusDrc* drcl, OpusRect* rcw) {
    const OpusRect rcl{drcl->x, drcl->y, drcl->x + drcl->dx,
                       drcl->y + drcl->dy};
    RclToRcw(hpldr, &rcl, rcw);
}

void DrcpToRcl(void** hpldr, int idr, const OpusDrc* drcp, OpusRect* rcl) {
    const OpusPoint origin = C_PtOrigin(hpldr, idr);
    rcl->left = drcp->x + origin.x;
    rcl->top = drcp->y + origin.y;
    rcl->right = rcl->left + drcp->dx;
    rcl->bottom = rcl->top + drcp->dy;
}

void DrcpToRcw(void** hpldr, int idr, const OpusDrc* drcp, OpusRect* rcw) {
    OpusRect rcl;
    DrcpToRcl(hpldr, idr, drcp, &rcl);
    const OpusPoint offset = WindowOffset({0, 0});
    TranslateRect(&rcl, rcw, offset.x, offset.y);
}

int XwFromXl(void** hpldr, int xl) {
    return xl + WindowOffset(C_PtOrigin(hpldr, -1)).x;
}

int YwFromYl(void** hpldr, int yl) {
    return yl + WindowOffset(C_PtOrigin(hpldr, -1)).y;
}

int YwFromYp(void** hpldr, int idr, int yp) {
    return yp + WindowOffset(C_PtOrigin(hpldr, idr)).y;
}

int YpFromYw(void** hpldr, int idr, int yw) {
    return yw - WindowOffset(C_PtOrigin(hpldr, idr)).y;
}

int XwFromXp(void** hpldr, int idr, int xp) {
    return xp + WindowOffset(C_PtOrigin(hpldr, idr)).x;
}

int XpFromXw(void** hpldr, int idr, int xw) {
    return xw - WindowOffset(C_PtOrigin(hpldr, idr)).x;
}

OpusRect* PrcSet(OpusRect* rect, int x, int y, int dx, int dy) {
    rect->left = x;
    rect->top = y;
    rect->right = x + dx;
    rect->bottom = y + dy;
    return rect;
}

int N_FormatLine(int ww, int doc, long cp) {
    C_FormatLine(ww, doc, cp);
    return 0;
}

int N_FormatDrLine(int ww, long cp, DR* pdr) {
    C_FormatDrLine(ww, cp, pdr);
    return 0;
}

int N_FormatLineDxa(int ww, int doc, long cp, int dxa) {
    C_FormatLineDxa(ww, doc, cp, dxa);
    return 0;
}

int N_FORMATLINEDXA(int ww, int doc, long cp, int dxa) {
    return N_FormatLineDxa(ww, doc, cp, dxa);
}

int N_ClFormatLines(void* plbs, long cp_lim, int dyl_fill, int dyl_max,
                    int cl_lim, int dxl, int ref_line,
                    int stop_at_page_break) {
    return C_ClFormatLines(plbs, cp_lim, dyl_fill, dyl_max, cl_lim, dxl,
                           ref_line, stop_at_page_break);
}

int N_FInsertRgch(int doc, long cp, char* text, int count, void* chp,
                  void* pap) {
    return C_FInsertRgch(doc, cp, text, count, chp, pap);
}

long N_FcAppendRgchToFn(int fn, char* text, int count) {
    return C_FcAppendRgchToFn(fn, text, count);
}

void N_ScanFnForBytes(int fn, char* text, unsigned count, int write) {
    C_ScanFnForBytes(fn, text, count, write);
}

int N_CbGrpprlProp(int normalized, char* output, int output_max,
                   void* properties, void* base_properties, int word_count,
                   void* property_map) {
    return C_CbGrpprlProp(normalized, output, output_max, properties,
                          base_properties, word_count, property_map);
}

int N_IfldFromDocCp(int doc, long cp, int match) {
    return C_IfldFromDocCp(doc, cp, match);
}

}  // extern "C"
