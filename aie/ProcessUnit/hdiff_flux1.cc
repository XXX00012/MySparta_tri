#include <adf.h>
#include "include.h"
#include "hdiff.h"

using namespace adf;

#define kernel_load 14

void hdiff_flux1(input_window_int32* row1_win,
                 input_window_int32* row2_win,
                 input_window_int32* row3_win,
                 input_window_int32* flux_forward1_win,
                 input_window_int32* flux_forward2_win,
                 input_window_int32* flux_forward3_win,
                 input_window_int32* flux_forward4_win,
                 output_window_int32* flux_inter1_win,
                 output_window_int32* flux_inter2_win,
                 output_window_int32* flux_inter3_win,
                 output_window_int32* flux_inter4_win,
                 output_window_int32* flux_inter5_win) {
  v16int32 data_buf1 = null_v16int32();
  v16int32 data_buf2 = null_v16int32();

  v8acc80 acc_0 = null_v8acc80();
  v8acc80 acc_1 = null_v8acc80();

  // standard-style preload
  data_buf1 = upd_w(data_buf1, 0, window_readincr_v8(row1_win));
  data_buf1 = upd_w(data_buf1, 1, window_read_v8(row1_win));

  data_buf2 = upd_w(data_buf2, 0, window_readincr_v8(row2_win));
  data_buf2 = upd_w(data_buf2, 1, window_read_v8(row2_win));

  for (unsigned i = 0; i < COL / 8; i++)
    chess_prepare_for_pipelining
    chess_loop_range(1, ) {
      v8int32 flux_sub;

      // flux_inter1
      flux_sub = window_readincr_v8(flux_forward1_win);
      acc_1 = lmul8(data_buf2, 2, 0x76543210, flux_sub, 0, 0x00000000);
      acc_1 = lmsc8(acc_1, data_buf2, 1, 0x76543210, flux_sub, 0, 0x00000000);
      window_writeincr(flux_inter1_win, flux_sub);
      window_writeincr(flux_inter1_win, srs(acc_1, 0));

      // flux_inter2
      flux_sub = window_readincr_v8(flux_forward2_win);
      acc_0 = lmul8(data_buf2, 3, 0x76543210, flux_sub, 0, 0x00000000);
      acc_0 = lmsc8(acc_0, data_buf2, 2, 0x76543210, flux_sub, 0, 0x00000000);
      window_writeincr(flux_inter2_win, flux_sub);
      window_writeincr(flux_inter2_win, srs(acc_0, 0));

      // flux_inter3
      flux_sub = window_readincr_v8(flux_forward3_win);
      acc_1 = lmul8(data_buf2, 2, 0x76543210, flux_sub, 0, 0x00000000);
      acc_1 = lmsc8(acc_1, data_buf1, 2, 0x76543210, flux_sub, 0, 0x00000000);
      window_writeincr(flux_inter3_win, flux_sub);
      window_writeincr(flux_inter3_win, srs(acc_1, 0));

      // row3 pair for flux_inter4
      data_buf1 = upd_w(data_buf1, 0, window_readincr_v8(row3_win));
      data_buf1 = upd_w(data_buf1, 1, window_read_v8(row3_win));

      // flux_inter4
      flux_sub = window_readincr_v8(flux_forward4_win);
      acc_1 = lmul8(data_buf1, 2, 0x76543210, flux_sub, 0, 0x00000000);
      acc_1 = lmsc8(acc_1, data_buf2, 2, 0x76543210, flux_sub, 0, 0x00000000);
      window_writeincr(flux_inter4_win, flux_sub);
      window_writeincr(flux_inter4_win, srs(acc_1, 0));

      // reload next row1 pair for next iteration
      data_buf1 = upd_w(data_buf1, 0, window_readincr_v8(row1_win));
      data_buf1 = upd_w(data_buf1, 1, window_read_v8(row1_win));

      // flux_inter5 keeps the same write order as standard-style buffered version
      window_writeincr(flux_inter5_win, ext_w(data_buf2, 1));
      window_writeincr(flux_inter5_win, ext_w(data_buf2, 0));

      // reload next row2 pair for next iteration
      data_buf2 = upd_w(data_buf2, 0, window_readincr_v8(row2_win));
      data_buf2 = upd_w(data_buf2, 1, window_read_v8(row2_win));
    }
}