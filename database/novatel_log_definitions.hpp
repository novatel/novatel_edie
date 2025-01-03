#include "decoders\common\api\common.hpp
#include "decoders\common\api\env.hpp"

#pragma pack(1)

struct PASSAUX
{
    ULONG buffer_arraylength;
    CHAR buffer[80];
};

struct TRANSFERPORTSTATUS
{
    LONG usb_detection_type;
    LONG usb_mode;
};

struct LOGLIST_log_list
{
    LONG log_port_address;
    ULONG message_id;
    LONG trigger;
    DOUBLE on_time;
    DOUBLE offset;
    LONG hold;
};

struct LOGLIST
{
    ULONG log_list_arraylength;
    LOGLIST_log_list log_list[80];
};

struct VERSION_num_components
{
    LONG type;
    CHAR model[16];
    CHAR psn[16];
    CHAR hw_version[16];
    CHAR sw_version[16];
    CHAR boot_version[16];
    CHAR compile_date[12];
    CHAR compile_time[12];
};

struct VERSION
{
    ULONG num_components_arraylength;
    VERSION_num_components num_components[20];
};

struct PORTSTATS_port_statistics
{
    LONG port;
    ULONG rx_chars;
    ULONG tx_chars;
    ULONG good_rx_chars;
    ULONG dropped_chars;
    ULONG interrupts;
    ULONG breaks;
    ULONG parity_errors;
    ULONG framing_errors;
    ULONG over_runs;
};

struct PORTSTATS
{
    ULONG port_statistics_arraylength;
    PORTSTATS_port_statistics port_statistics[33];
};

struct RXSTATUS_num_stats
{
    ULONG rxstat;
    ULONG rxstat_pri;
    ULONG rxstat_set;
    ULONG rxstat_clear;
};

struct RXSTATUS
{
    ULONG error;
    ULONG num_stats_arraylength;
    RXSTATUS_num_stats num_stats[5];
};

struct RXSTATUSEVENT
{
    LONG word;
    ULONG bit_position;
    LONG event;
    CHAR description[32];
};

struct RXCONFIG
{
    ULONG message_id;
    ULONG message_length;
    UCHAR message_buffer[450];
};

struct VALIDMODELS_models
{
    UCHAR model[16];
    ULONG expiry_year;
    ULONG expiry_month;
    ULONG expiry_day;
};

struct VALIDMODELS
{
    ULONG models_arraylength;
    VALIDMODELS_models models[24];
};

struct HWMONITOR_measurements
{
    FLOAT value;
    ULONG status;
};

struct HWMONITOR
{
    ULONG measurements_arraylength;
    HWMONITOR_measurements measurements[32];
};

struct CHANCONFIGLIST_chan_cfg_list_array_chan_cfg_list_sys_array
{
    ULONG num_chans;
    LONG signal_type;
};

struct CHANCONFIGLIST_chan_cfg_list_array
{
    ULONG chan_cfg_list_sys_array_arraylength;
    CHANCONFIGLIST_chan_cfg_list_array_chan_cfg_list_sys_array chan_cfg_list_sys_array[16];
};

struct CHANCONFIGLIST
{
    ULONG set_in_use;
    ULONG chan_cfg_list_array_arraylength;
    CHANCONFIGLIST_chan_cfg_list_array chan_cfg_list_array[20];
};

struct PASSETH1
{
    ULONG buffer_arraylength;
    CHAR buffer[80];
};

struct SOFTLOADSTATUS
{
    LONG status;
};

struct ETHSTATUS_eth_status
{
    LONG interface;
    UCHAR mac_address[18];
    LONG interface_config;
};

struct ETHSTATUS
{
    ULONG eth_status_arraylength;
    ETHSTATUS_eth_status eth_status[2];
};

struct IPSTATUS_ip_status
{
    LONG interface;
    UCHAR ip_address[16];
    UCHAR netmask[16];
    UCHAR gateway[16];
};

struct IPSTATUS_dns_server
{
    UCHAR ip_address_6[16];
};

struct IPSTATUS
{
    ULONG ip_status_arraylength;
    IPSTATUS_ip_status ip_status[4];
    ULONG dns_server_arraylength;
    IPSTATUS_dns_server dns_server[4];
};

struct MODELFEATURES_features
{
    LONG feature_status;
    LONG feature;
};

struct MODELFEATURES
{
    ULONG features_arraylength;
    MODELFEATURES_features features[30];
};

struct PASSTHROUGH
{
    LONG port;
    ULONG buffer_arraylength;
    CHAR buffer[80];
};

struct SOURCETABLE
{
    UCHAR endpoint[80];
    ULONG SOURCETABLE_reserved1;
    ULONG SOURCETABLE_reserved2;
    UCHAR entry_data[512];
};

struct PROFILEINFO_profile_info_command
{
    UCHAR command[201];
};

struct PROFILEINFO
{
    UCHAR profile_name[20];
    ULONG status;
    ULONG profile_info_command_arraylength;
    PROFILEINFO_profile_info_command profile_info_command[20];
};

struct IPSTATS_ip_interface_statistics
{
    LONG physical_interface;
    ULONG connection_duration;
    ULONG rx_count;
    ULONG tx_count;
};

struct IPSTATS
{
    ULONG ip_interface_statistics_arraylength;
    IPSTATS_ip_interface_statistics ip_interface_statistics[24];
};

struct UPTIME
{
    ULONG uptime;
};

struct RADARSTATUS
{
    ULONG status;
    LONG solution_status;
    LONG solution_type;
    DOUBLE horizontal_speed_mps;
    DOUBLE smoothed_horizontal_speed_mps;
    DOUBLE frequency;
};

struct J1939STATUS
{
    LONG j1939_node;
    LONG j1939_node_status;
    ULONG address_claim_count;
    UCHAR claimed_address;
};

struct SAFEMODESTATUS
{
    LONG safe_mode_status;
    ULONG reset_count;
    UCHAR description[80];
};

struct FILELIST
{
    LONG mass_storage_device;
    LONG entry_type;
    ULONG file_size;
    ULONG last_change_date;
    ULONG last_change_time;
    CHAR file_name[128];
};

struct FILETRANSFERSTATUS
{
    LONG file_transfer_status;
    ULONG total_transferred;
    ULONG total_transfer_size;
    UCHAR file_name[128];
    UCHAR error_msg[256];
};

struct FILESYSTEMSTATUS
{
    LONG mass_storage_device;
    LONG file_system_status;
    ULONG media_capacity;
    UCHAR error_message[64];
};

struct FILESTATUS
{
    LONG media_type;
    LONG file_status;
    UCHAR file_name[128];
    ULONG file_size;
    ULONG media_remaining_capacity;
    ULONG media_total_capacity;
    UCHAR error_msg_string[128];
};

struct FILESYSTEMCAPACITY_file_systems
{
    LONG file_system_type;
    ULONGLONG capacity;
    ULONGLONG used_capacity;
};

struct FILESYSTEMCAPACITY
{
    ULONG file_systems_arraylength;
    FILESYSTEMCAPACITY_file_systems file_systems[3];
};

struct LUAFILESYSTEMSTATUS
{
    LONG file_system_status;
    UCHAR error[52];
};

struct LUAFILELIST
{
    ULONG file_size;
    ULONG last_changed_date;
    ULONG last_changed_time;
    UCHAR file_path[256];
};

struct LUASTATUS
{
    ULONG executor_number;
    UCHAR script_command_line[400];
    LONG lua_status;
};

struct USERI2CRESPONSE
{
    UCHAR slave_device_address;
    ULONG slave_register_address;
    LONG status_code;
    LONG operation_mode;
    ULONG transaction_id;
    ULONG read_data_arraylength;
    HEXBYTE read_data[256];
};

struct LUAOUTPUT
{
    ULONG sequence_number;
    ULONG lua_executor;
    LONG source;
    UCHAR data[128];
};

struct PPPSEEDSTORESTATUS
{
    LONG status;
    FLOAT horizontal_std_dev;
};

struct USERANTENNA_pc_cs
{
    LONG frequency;
    FLOAT pco[3];
    FLOAT pcv[19];
};

struct USERANTENNA
{
    LONG antenna;
    CHAR antenna_name[16];
    ULONG pc_cs_arraylength;
    USERANTENNA_pc_cs pc_cs[24];
};

struct GEODETICDATUMS_datums
{
    CHAR name[32];
    ULONG epsg_code;
    LONG anchor;
    DOUBLE semi_major_axis;
    DOUBLE inverse_flattening;
};

struct GEODETICDATUMS
{
    ULONG datums_arraylength;
    GEODETICDATUMS_datums datums[64];
};

struct DATUMTRANSFORMATIONS_transformations
{
    CHAR name[32];
    CHAR name_2[32];
    DOUBLE epoch;
    FLOAT x_translation;
    FLOAT y_translation;
    FLOAT z_translation;
    FLOAT x_rotation;
    FLOAT y_rotation;
    FLOAT z_rotation;
    FLOAT scale_difference;
    FLOAT x_translation_rate;
    FLOAT y_translation_rate;
    FLOAT z_translation_rate;
    FLOAT x_rotation_rate;
    FLOAT y_rotation_rate;
    FLOAT z_rotation_rate;
    FLOAT scale_difference_rate;
};

struct DATUMTRANSFORMATIONS
{
    ULONG transformations_arraylength;
    DATUMTRANSFORMATIONS_transformations transformations[63];
};

struct GPSEPHEM
{
    ULONG PRN;
    DOUBLE tow;
    ULONG health;
    ULONG IODE1;
    ULONG IODE2;
    ULONG GPSEPHEM_week;
    ULONG z_week;
    DOUBLE toe;
    DOUBLE A;
    DOUBLE deltaN;
    DOUBLE M_0;
    DOUBLE ecc;
    DOUBLE w;
    DOUBLE cuc;
    DOUBLE cus;
    DOUBLE crc;
    DOUBLE crs;
    DOUBLE cic;
    DOUBLE cis;
    DOUBLE I_0;
    DOUBLE I_exp0;
    DOUBLE w_o;
    DOUBLE w_dot;
    ULONG iodc;
    DOUBLE toc;
    DOUBLE tgd;
    DOUBLE a_f0;
    DOUBLE a_f1;
    DOUBLE a_f2;
    BOOL AS;
    DOUBLE N;
    DOUBLE URA;
};

struct IONUTC
{
    DOUBLE a0;
    DOUBLE a1;
    DOUBLE a2;
    DOUBLE a3;
    DOUBLE b0;
    DOUBLE b1;
    DOUBLE b2;
    DOUBLE b3;
    ULONG w_nt;
    ULONG tot;
    DOUBLE a0_10;
    DOUBLE a1_11;
    ULONG w_nlsf;
    ULONG dn;
    LONG delta_tls;
    LONG delta_tlsf;
    ULONG delta_tutc;
};

struct RAWGPSSUBFRAME
{
    INT frame_decoder_number;
    ULONG satellite_id;
    ULONG sub_frame_id;
    HEXBYTE raw_sub_frame_data[30];
    ULONG signal_channel_number;
};

struct CLOCKSTEERING
{
    LONG steering_source;
    LONG steering_state;
    ULONG modulus;
    DOUBLE effective_pulse_width;
    DOUBLE bandwidth;
    FLOAT slope;
    DOUBLE last_offset;
    DOUBLE last_rate;
};

struct RAWEPHEM
{
    ULONG prn;
    ULONG ref_week;
    ULONG ref_secs;
    HEXBYTE subframe1[30];
    HEXBYTE subframe2[30];
    HEXBYTE subframe3[30];
};

struct RANGE_num_obs
{
    USHORT PRN_slot;
    USHORT glofreq;
    DOUBLE psr;
    FLOAT psr_std_dev;
    DOUBLE adr;
    FLOAT adr_std_dev;
    FLOAT dopp;
    FLOAT C_No;
    FLOAT lock_time;
    ULONG ch_tr_status;
};

struct RANGE
{
    ULONG num_obs_arraylength;
    RANGE_num_obs num_obs[325];
};

struct ALMANAC_num_messages
{
    ULONG PRN;
    ULONG ALMANAC_week;
    DOUBLE ALMANAC_seconds;
    DOUBLE ecc;
    DOUBLE w_dot;
    DOUBLE w_0;
    DOUBLE w;
    DOUBLE M_0;
    DOUBLE a_f0;
    DOUBLE a_f1;
    DOUBLE N_0;
    DOUBLE A;
    DOUBLE incl_angle;
    ULONG sv_config;
    ULONG health_prn;
    ULONG health_alm;
    BOOL antispoof;
};

struct ALMANAC
{
    ULONG num_messages_arraylength;
    ALMANAC_num_messages num_messages[32];
};

struct RAWALM_sub_frame_pages
{
    USHORT svid;
    HEXBYTE page_raw_data[30];
};

struct RAWALM
{
    ULONG weeks;
    ULONG milliseconds;
    ULONG sub_frame_pages_arraylength;
    RAWALM_sub_frame_pages sub_frame_pages[46];
};

struct TRACKSTAT_num_channels
{
    USHORT PRN_slot;
    USHORT glofreq;
    ULONG ch_tr_status;
    DOUBLE psr;
    FLOAT doppler;
    FLOAT C_No;
    FLOAT locktime;
    FLOAT psr_res;
    LONG reject;
    FLOAT psr_weight;
};

struct TRACKSTAT
{
    LONG solution_status;
    LONG position_type;
    FLOAT cutoff;
    ULONG num_channels_arraylength;
    TRACKSTAT_num_channels num_channels[325];
};

struct TIME
{
    LONG clock_model_status;
    DOUBLE offset;
    DOUBLE offset_std;
    DOUBLE utc_offset;
    ULONG utc_year;
    UCHAR utc_month;
    UCHAR utc_day;
    UCHAR utc_hour;
    UCHAR utc_minute;
    ULONG utc_millisecond;
    LONG utc_time_status;
};

struct RANGECMP_range_c_entry
{
    HEXBYTE range_c_data[24];
};

struct RANGECMP
{
    ULONG range_c_entry_arraylength;
    RANGECMP_range_c_entry range_c_entry[325];
};

struct RAWGPSWORD
{
    ULONG prn;
    ULONG raw_word;
};

struct TIMESYNC
{
    ULONG gps_week;
    ULONG milliseconds;
    LONG time_status;
};

struct RANGEGPSL1_obs
{
    USHORT sv_prn;
    USHORT sv_freq;
    DOUBLE psr;
    FLOAT sd_psr;
    DOUBLE adr;
    FLOAT sd_adr;
    FLOAT dop;
    FLOAT C_No;
    FLOAT lock_time;
    ULONG c_status;
};

struct RANGEGPSL1
{
    ULONG obs_arraylength;
    RANGEGPSL1_obs obs[325];
};

struct GLOALMANAC_num_recs
{
    ULONG GLOALMANAC_week;
    ULONG GLOALMANAC_time;
    UCHAR slot;
    CHAR frequency;
    UCHAR sat_type;
    UCHAR health;
    DOUBLE TlambdaN;
    DOUBLE lambdaN;
    DOUBLE deltaI;
    DOUBLE ecc;
    DOUBLE arg_perig;
    DOUBLE detlaT;
    DOUBLE deltaTD;
    DOUBLE tau;
};

struct GLOALMANAC
{
    ULONG num_recs_arraylength;
    GLOALMANAC_num_recs num_recs[24];
};

struct GLOCLOCK
{
    ULONG nominal_offset;
    DOUBLE residual_offset;
    DOUBLE residual_offset_var;
    UCHAR sat_type;
    UCHAR n4;
    DOUBLE tau_gps;
    USHORT na;
    DOUBLE tau_c;
    DOUBLE b1;
    DOUBLE b2;
    UCHAR kp;
};

struct GLORAWALM_num_recs
{
    HEXBYTE string[11];
    UCHAR GLORAWALM_reserved1;
};

struct GLORAWALM
{
    ULONG GLORAWALM_week;
    ULONG GLORAWALM_time;
    ULONG num_recs_arraylength;
    GLORAWALM_num_recs num_recs[54];
};

struct GLORAWFRAME_raw_string
{
    HEXBYTE string[11];
    UCHAR GLORAWFRAME_reserved;
};

struct GLORAWFRAME
{
    ULONG frame_number;
    USHORT sloto;
    USHORT freqo;
    ULONG weeks;
    ULONG milliseconds;
    ULONG frame_decoder_number;
    ULONG signal_channel_number;
    ULONG raw_string_arraylength;
    GLORAWFRAME_raw_string raw_string[15];
};

struct GLORAWSTRING
{
    UCHAR slot;
    CHAR freq;
    HEXBYTE string[11];
    UCHAR GLORAWSTRING_reserved;
};

struct GLOEPHEMERIS
{
    USHORT slot_o;
    USHORT freq_o;
    UCHAR sat_type;
    UCHAR GLOEPHEMERIS_reserved1;
    USHORT e_week;
    ULONG e_time;
    ULONG t_offset;
    USHORT Nt;
    UCHAR GLOEPHEMERIS_reserved2;
    UCHAR GLOEPHEMERIS_reserved3;
    ULONG issue;
    ULONG health;
    DOUBLE pos_x;
    DOUBLE pos_y;
    DOUBLE pos_z;
    DOUBLE vel_x;
    DOUBLE vel_y;
    DOUBLE vel_z;
    DOUBLE LS_acc_x;
    DOUBLE LS_acc_y;
    DOUBLE LS_acc_z;
    DOUBLE tau_n;
    DOUBLE delta_tau_n;
    DOUBLE gamma;
    ULONG Tk;
    ULONG p;
    ULONG Ft;
    ULONG age;
    ULONG flags;
};

struct GLORAWEPHEM_num_recs
{
    HEXBYTE string[11];
    UCHAR GLORAWEPHEM_reserved1;
};

struct GLORAWEPHEM
{
    USHORT slot_o;
    USHORT freq_o;
    ULONG sig_chan;
    ULONG GLORAWEPHEM_week;
    ULONG GLORAWEPHEM_time;
    ULONG num_recs_arraylength;
    GLORAWEPHEM_num_recs num_recs[4];
};

struct RAWSBASFRAME
{
    INT frame_decoder_num;
    ULONG prn;
    ULONG waas_msg_id;
    HEXBYTE raw_frame_data[29];
    ULONG signal_channel_num;
};

struct SBAS0
{
    ULONG prn;
};

struct SBAS1
{
    ULONG prn;
    HEXBYTE prn_mask[27];
    ULONG iodp;
};

struct SBAS10
{
    ULONG prn;
    ULONG brrc;
    ULONG cltc_lsb;
    ULONG cltc_v1;
    ULONG iltc_v1;
    ULONG cltc_v0;
    ULONG iltc_v0;
    ULONG cgeo_lsb;
    ULONG cgeo_v;
    ULONG igeo;
    ULONG cer;
    ULONG ciono_step;
    ULONG iiono;
    ULONG ciono_ramp;
    ULONG rssudre;
    ULONG rss_iono;
    HEXBYTE spare_bits[11];
};

struct SBAS12
{
    ULONG prn;
    DOUBLE a1;
    DOUBLE a0;
    ULONG t0t;
    USHORT wn;
    SHORT dt_ls;
    USHORT wnlsf;
    USHORT dn;
    SHORT dt_lsf;
    USHORT utcid;
    ULONG gpstow;
    ULONG gpswn;
    BOOL glonass_indicator;
    HEXBYTE SBAS12_reserved_bits[10];
};

struct SBAS17_items
{
    USHORT data_id;
    USHORT prn_3;
    USHORT health;
    LONG x;
    LONG y;
    LONG z;
    LONG x_vel;
    LONG y_vel;
    LONG z_vel;
};

struct SBAS17
{
    ULONG prn;
    ULONG items_arraylength;
    SBAS17_items items[3];
    ULONG t0;
};

struct SBAS18
{
    ULONG prn;
    ULONG num_bands;
    ULONG band_num;
    ULONG iodi;
    HEXBYTE igp_mask[26];
    ULONG spare_bit;
};

struct SBAS2
{
    ULONG prn;
    ULONG iodf;
    ULONG iodp;
    LONG prc[13];
    ULONG udrei[13];
};

struct SBAS24
{
    ULONG prn;
    LONG prc[6];
    ULONG udrei[6];
    ULONG iodp;
    ULONG block_id;
    ULONG iodf;
    ULONG spare;
    ULONG velocity_code;
    ULONG prn_mask_number1;
    ULONG iode1;
    LONG dx1;
    LONG dy1;
    LONG dz1;
    LONG a_f01;
    ULONG prn_mask_number2;
    ULONG iode2;
    LONG dx2or_ddx;
    LONG dy2or_ddy;
    LONG dz2or_ddz;
    LONG a_f02ora_f1;
    ULONG tod;
    ULONG iodp_21;
    ULONG spare_22;
};

struct SBAS25
{
    ULONG prn;
    ULONG velocity_code;
    ULONG prn_mask_number1;
    ULONG iode1;
    LONG dx1;
    LONG dy1;
    LONG dz1;
    LONG a_f01;
    ULONG prn_mask_number2;
    ULONG iode2;
    LONG dx2or_ddx;
    LONG dy2or_ddy;
    LONG dz2or_ddz;
    LONG a_f02ora_f1;
    ULONG tod;
    ULONG iodp;
    ULONG spare;
    ULONG velocity_code_17;
    ULONG prn_mask_number1_18;
    ULONG iode1_19;
    LONG dx1_20;
    LONG dy1_21;
    LONG dz1_22;
    LONG a_f01_23;
    ULONG prn_mask_number2_24;
    ULONG iode2_25;
    LONG dx2or_ddx_26;
    LONG dy2or_ddy_27;
    LONG dz2or_ddz_28;
    LONG a_f02ora_f1_29;
    ULONG tod_30;
    ULONG iodp_31;
    ULONG spare_32;
};

struct SBAS26_grid_point_data
{
    ULONG igpvde;
    ULONG givei;
};

struct SBAS26
{
    ULONG prn;
    ULONG band_num;
    ULONG block_id;
    ULONG grid_point_data_arraylength;
    SBAS26_grid_point_data grid_point_data[15];
    ULONG iodi;
    ULONG spare_bits;
};

struct SBAS27_regions
{
    LONG lat1;
    LONG long1;
    LONG lat2;
    LONG long2;
    ULONG shape;
};

struct SBAS27
{
    ULONG prn;
    ULONG iods;
    ULONG num_service_msgs;
    ULONG service_msg_num;
    ULONG priority_code;
    ULONG udre_inside;
    ULONG udre_outside;
    ULONG regions_arraylength;
    SBAS27_regions regions[5];
    ULONG SBAS27_reserved;
};

struct SBAS3
{
    ULONG prn;
    ULONG iodf;
    ULONG iodp;
    LONG prc[13];
    ULONG udrei[13];
};

struct SBAS4
{
    ULONG prn;
    ULONG iodf;
    ULONG iodp;
    LONG prc[13];
    ULONG udrei[13];
};

struct SBAS5
{
    ULONG prn;
    ULONG iodf;
    ULONG iodp;
    LONG prc[13];
    ULONG udrei[13];
};

struct SBAS6
{
    ULONG prn;
    ULONG iodf2;
    ULONG iodf3;
    ULONG iodf4;
    ULONG iodf5;
    ULONG udrei[51];
};

struct SBAS7
{
    ULONG prn;
    ULONG system_latency;
    ULONG iodp;
    ULONG spare_bits;
    ULONG degradation_factor[51];
};

struct SBAS9
{
    ULONG prn;
    ULONG iodn;
    ULONG t0;
    ULONG ura;
    DOUBLE x;
    DOUBLE y;
    DOUBLE z;
    DOUBLE x_vel;
    DOUBLE y_vel;
    DOUBLE z_vel;
    DOUBLE x_accel;
    DOUBLE y_accel;
    DOUBLE z_accel;
    DOUBLE af0;
    DOUBLE af1;
};

struct SATVIS2_sat_vis_list
{
    SATELLITEID id;
    ULONG sat_health;
    DOUBLE elevation;
    DOUBLE azimuth;
    DOUBLE true_doppler;
    DOUBLE apparent_doppler;
};

struct SATVIS2
{
    LONG system_type;
    BOOL is_sat_vis_valid;
    BOOL was_gnss_almanac_used;
    ULONG sat_vis_list_arraylength;
    SATVIS2_sat_vis_list sat_vis_list[63];
};

struct RAWCNAVFRAME
{
    ULONG sig_chan_num;
    ULONG prn;
    ULONG frame_id;
    HEXBYTE raw_frame_data[38];
};

struct MARK3TIME
{
    LONG week;
    DOUBLE seconds;
    DOUBLE offset;
    DOUBLE offset_std;
    DOUBLE utc_offset;
    LONG status;
};

struct MARK4TIME
{
    LONG week;
    DOUBLE seconds;
    DOUBLE offset;
    DOUBLE offset_std;
    DOUBLE utc_offset;
    LONG status;
};

struct GALALMANAC
{
    ULONG sat_id;
    BOOL fnav_received;
    BOOL inav_received;
    UCHAR E1B_health;
    UCHAR E5a_health;
    UCHAR E5b_health;
    UCHAR GALALMANAC_reserved1;
    ULONG ioda;
    ULONG GALALMANAC_week;
    ULONG GALALMANAC_seconds;
    DOUBLE ecc;
    DOUBLE omega_dot;
    DOUBLE omega_0;
    DOUBLE omega;
    DOUBLE M_0;
    DOUBLE A_f0;
    DOUBLE A_f1;
    DOUBLE delta_rootA;
    DOUBLE deltaI;
};

struct GALCLOCK
{
    DOUBLE A_0;
    DOUBLE A_1;
    LONG delta_T_ls;
    ULONG T_ot;
    ULONG WN_t;
    ULONG WN_lsf;
    ULONG dn;
    LONG delta_T_lsf;
    DOUBLE A_0g;
    DOUBLE A_1g;
    ULONG T_0g;
    ULONG WN_0g;
};

struct GALFNAVRAWALMANAC
{
    ULONG IODa;
    ULONG WNa;
    ULONG T0a;
    HEXBYTE raw_data[20];
};

struct GALFNAVRAWEPHEMERIS_num_recs
{
    HEXBYTE raw_data[27];
    UCHAR GALFNAVRAWEPHEMERIS_reserved1;
};

struct GALFNAVRAWEPHEMERIS
{
    ULONG sat_id;
    ULONG GALFNAVRAWEPHEMERIS_weeks;
    ULONG GALFNAVRAWEPHEMERIS_time;
    ULONG num_recs_arraylength;
    GALFNAVRAWEPHEMERIS_num_recs num_recs[4];
};

struct GALINAVRAWALMANAC
{
    ULONG IODa;
    ULONG WNa;
    ULONG T0a;
    HEXBYTE raw_data[20];
};

struct GALINAVRAWEPHEMERIS_num_recs
{
    HEXBYTE raw_data[16];
};

struct GALINAVRAWEPHEMERIS
{
    ULONG sat_id;
    ULONG GALINAVRAWEPHEMERIS_weeks;
    ULONG GALINAVRAWEPHEMERIS_time;
    ULONG num_recs_arraylength;
    GALINAVRAWEPHEMERIS_num_recs num_recs[6];
};

struct GALIONO
{
    DOUBLE ai0;
    DOUBLE ai1;
    DOUBLE ai2;
    UCHAR sf1;
    UCHAR sf2;
    UCHAR sf3;
    UCHAR sf4;
    UCHAR sf5;
};

struct MARK1TIME
{
    LONG week;
    DOUBLE seconds;
    DOUBLE offset;
    DOUBLE offset_std;
    DOUBLE utc_offset;
    LONG status;
};

struct LBANDTRACKSTAT_l_band_chan_states
{
    CHAR beam_name[8];
    ULONG assigned_frequency;
    USHORT baud_rate;
    USHORT service_id;
    USHORT tracking_status;
    USHORT LBANDTRACKSTAT_reserved;
    FLOAT doppler;
    FLOAT cn0;
    FLOAT phase_std_dev;
    FLOAT lock_time;
    ULONG total_unique_word_bits;
    ULONG bad_unique_word_bits;
    ULONG bad_unique_words;
    ULONG total_viterbi_symbols;
    ULONG corrected_viterbi_syms;
    FLOAT ber;
};

struct LBANDTRACKSTAT
{
    ULONG l_band_chan_states_arraylength;
    LBANDTRACKSTAT_l_band_chan_states l_band_chan_states[5];
};

struct RANGECMP2
{
    ULONG range_data_arraylength;
    HEXBYTE range_data[7800];
};

struct GALINAVEPHEMERIS
{
    ULONG sat_id;
    UCHAR E5b_health;
    UCHAR E5b_DVS;
    UCHAR GALINAVEPHEMERIS_reserved1;
    UCHAR GALINAVEPHEMERIS_reserved2;
    UCHAR E1b_health;
    UCHAR E1b_DVS;
    UCHAR GALINAVEPHEMERIS_reserved3;
    UCHAR GALINAVEPHEMERIS_reserved4;
    USHORT IOD_nav;
    UCHAR SISA_index;
    UCHAR INAV_source;
    UINT T0e;
    ULONG T0c;
    DOUBLE M0;
    DOUBLE delta_N;
    DOUBLE Ecc;
    DOUBLE root_A;
    DOUBLE I0;
    DOUBLE IDot;
    DOUBLE omega_0;
    DOUBLE omega;
    DOUBLE omega_dot;
    DOUBLE Cuc;
    DOUBLE Cus;
    DOUBLE Crc;
    DOUBLE Crs;
    DOUBLE Cic;
    DOUBLE Cis;
    DOUBLE Af0;
    DOUBLE Af1;
    DOUBLE Af2;
    DOUBLE E1E5aBGD;
    DOUBLE E1E5bBGD;
};

struct GALFNAVEPHEMERIS
{
    ULONG sat_id;
    UCHAR E5a_health;
    UCHAR E5a_DVS;
    UCHAR GALFNAVEPHEMERIS_reserved1;
    UCHAR GALFNAVEPHEMERIS_reserved2;
    USHORT IOD_nav;
    UCHAR SISA_index;
    UCHAR GALFNAVEPHEMERIS_reserved3;
    UINT T0e;
    ULONG T0c;
    DOUBLE M0;
    DOUBLE delta_N;
    DOUBLE Ecc;
    DOUBLE root_A;
    DOUBLE I0;
    DOUBLE IDot;
    DOUBLE omega_0;
    DOUBLE omega;
    DOUBLE omega_dot;
    DOUBLE Cuc;
    DOUBLE Cus;
    DOUBLE Crc;
    DOUBLE Crs;
    DOUBLE Cic;
    DOUBLE Cis;
    DOUBLE Af0;
    DOUBLE Af1;
    DOUBLE Af2;
    DOUBLE E1E5aBGD;
};

struct QZSSRAWSUBFRAME
{
    ULONG satellite_id;
    ULONG sub_frame_id;
    HEXBYTE raw_sub_frame_data[30];
    ULONG signal_channel_number;
};

struct QZSSRAWEPHEM
{
    ULONG prn;
    ULONG ref_week;
    ULONG ref_secs;
    HEXBYTE subframe1[30];
    HEXBYTE subframe2[30];
    HEXBYTE subframe3[30];
};

struct QZSSEPHEMERIS
{
    ULONG satellite_id;
    DOUBLE tow;
    ULONG health6;
    ULONG iode1;
    ULONG iode2;
    ULONG wn;
    ULONG zwn;
    DOUBLE toe;
    DOUBLE a;
    DOUBLE delta_n;
    DOUBLE m0;
    DOUBLE ecc;
    DOUBLE omega;
    DOUBLE cuc;
    DOUBLE cus;
    DOUBLE crc;
    DOUBLE crs;
    DOUBLE cic;
    DOUBLE cis;
    DOUBLE i0;
    DOUBLE i_dot;
    DOUBLE omega0;
    DOUBLE omega_dot;
    ULONG iodc;
    DOUBLE toc;
    DOUBLE tgd;
    DOUBLE af0;
    DOUBLE af1;
    DOUBLE af2;
    BOOL anti_spoofing;
    DOUBLE n;
    DOUBLE eph_var;
    UCHAR fit_interval;
    UCHAR char_as_int;
    UCHAR char_as_int_34;
    UCHAR char_as_int_35;
};

struct QZSSRAWALMANAC_num_subframes
{
    USHORT svid;
    HEXBYTE data[30];
};

struct QZSSRAWALMANAC
{
    ULONG ref_week;
    ULONG ref_secs;
    ULONG num_subframes_arraylength;
    QZSSRAWALMANAC_num_subframes num_subframes[46];
};

struct QZSSALMANAC_sv_alm_data
{
    ULONG prn;
    ULONG wn;
    DOUBLE toa;
    DOUBLE ecc;
    DOUBLE omega_dot;
    DOUBLE omega0;
    DOUBLE omega;
    DOUBLE mo;
    DOUBLE af0;
    DOUBLE af1;
    DOUBLE n;
    DOUBLE a;
    DOUBLE di;
    ULONG health6;
    ULONG health8;
};

struct QZSSALMANAC
{
    ULONG sv_alm_data_arraylength;
    QZSSALMANAC_sv_alm_data sv_alm_data[10];
};

struct QZSSIONUTC
{
    DOUBLE a0;
    DOUBLE a1;
    DOUBLE a2;
    DOUBLE a3;
    DOUBLE b0;
    DOUBLE b1;
    DOUBLE b2;
    DOUBLE b3;
    ULONG w_nt;
    ULONG tot;
    DOUBLE a0_10;
    DOUBLE a1_11;
    ULONG w_nlsf;
    ULONG dn;
    LONG delta_tls;
    LONG delta_tlsf;
    ULONG delta_tutc;
};

struct AUTHCODES_auth_codes
{
    LONG type;
    BOOL valid;
    UCHAR auth_code[80];
};

struct AUTHCODES
{
    LONG signature_status;
    ULONG auth_codes_arraylength;
    AUTHCODES_auth_codes auth_codes[24];
};

struct GALFNAVRAWPAGE
{
    ULONG sig_chan_num;
    ULONG sat_id;
    HEXBYTE raw_frame_data[27];
};

struct GALINAVRAWWORD
{
    ULONG sig_chan_num;
    ULONG sat_id;
    LONG signal_type;
    HEXBYTE raw_frame_data[16];
};

struct SATXYZ2_sats
{
    LONG system_type;
    SATELLITEID id;
    DOUBLE x;
    DOUBLE y;
    DOUBLE z;
    DOUBLE rb;
    DOUBLE iono_corr;
    DOUBLE tropo_corr;
    DOUBLE dummy;
    DOUBLE dummy_10;
};

struct SATXYZ2
{
    ULONG sats_arraylength;
    SATXYZ2_sats sats[72];
};

struct QZSSRAWCNAVMESSAGE
{
    ULONG sig_chan_num;
    ULONG prn;
    ULONG message_id;
    HEXBYTE raw_frame_data[38];
};

struct BDSALMANAC
{
    ULONG satellite_id;
    ULONG BDSALMANAC_week;
    ULONG toa;
    DOUBLE rootA;
    DOUBLE ecc;
    DOUBLE w;
    DOUBLE M_0;
    DOUBLE omega;
    DOUBLE omega_dot;
    DOUBLE d_i;
    DOUBLE a_0;
    DOUBLE a_1;
    ULONG health;
};

struct BDSIONO
{
    ULONG transmitting_satellite_id;
    DOUBLE alpha0;
    DOUBLE alpha1;
    DOUBLE alpha2;
    DOUBLE alpha3;
    DOUBLE beta0;
    DOUBLE beta1;
    DOUBLE beta2;
    DOUBLE beta3;
};

struct BDSCLOCK
{
    DOUBLE A_0utc;
    DOUBLE A_1utc;
    SHORT deltaT_LS;
    USHORT WN_lsf;
    USHORT dn;
    SHORT deltaT_lsf;
    DOUBLE A_0gps;
    DOUBLE A_1gps;
    DOUBLE A_0gal;
    DOUBLE A_1gal;
    DOUBLE A_0glo;
    DOUBLE A_1glo;
};

struct BDSRAWNAVSUBFRAME
{
    ULONG signal_channel_number;
    ULONG satellite_id;
    LONG bds_data_source;
    ULONG subframe_id;
    HEXBYTE raw_subframe_data[28];
};

struct BDSEPHEMERIS
{
    ULONG satellite_id;
    ULONG BDSEPHEMERIS_week;
    DOUBLE ura;
    ULONG health;
    DOUBLE tgd1;
    DOUBLE tgd2;
    ULONG aodc;
    ULONG toc;
    DOUBLE a_0;
    DOUBLE a_1;
    DOUBLE a_2;
    ULONG aode;
    ULONG toe;
    DOUBLE rootA;
    DOUBLE ecc;
    DOUBLE w;
    DOUBLE deltaN;
    DOUBLE M_0;
    DOUBLE omega_0;
    DOUBLE omega_dot;
    DOUBLE i_0;
    DOUBLE idot;
    DOUBLE c_uc;
    DOUBLE c_us;
    DOUBLE c_rc;
    DOUBLE c_rs;
    DOUBLE c_ic;
    DOUBLE c_is;
};

struct RANGECMP3
{
    ULONG range_data_arraylength;
    HEXBYTE range_data[16250];
};

struct ITPSDFINAL
{
    ULONG status_word;
    FLOAT frequency_start_m_hz;
    FLOAT step_size_hz;
    ULONG samples_arraylength;
    USHORT samples[1024];
};

struct ITFILTTABLE_filter_coef_status_nf_status
{
    LONG enabled_9;
    LONG pfid;
    LONG mode;
    FLOAT lower_cut_off_frequency;
    FLOAT higher_cut_off_frequency;
    FLOAT frequency_width;
};

struct ITFILTTABLE_filter_coef_status
{
    LONG frequency;
    ULONG encoder_id;
    LONG ddc_filter_type;
    ULONG status;
    LONG enabled;
    FLOAT lower_cut_off_freqency;
    FLOAT higher_cut_off_freqency;
    ULONG nf_status_arraylength;
    ITFILTTABLE_filter_coef_status_nf_status nf_status[3];
};

struct ITFILTTABLE
{
    ULONG filter_coef_status_arraylength;
    ITFILTTABLE_filter_coef_status filter_coef_status[25];
};

struct ITBANDPASSBANK_bpf_bank_entries
{
    LONG frequency;
    FLOAT min_lower_cut_off_frequency;
    FLOAT max_lower_cut_off_frequency;
    FLOAT min_higher_cut_off_frequency;
    FLOAT max_higher_cut_off_frequency;
    FLOAT frequency_step;
};

struct ITBANDPASSBANK
{
    ULONG bpf_bank_entries_arraylength;
    ITBANDPASSBANK_bpf_bank_entries bpf_bank_entries[12];
};

struct ITPROGFILTBANK_nf_bank_entries_nf_parameters
{
    LONG nf_mode;
    FLOAT min_lower_cut_off_frequency;
    FLOAT max_lower_cut_off_frequency;
    FLOAT min_higher_cut_off_frequency;
    FLOAT max_higher_cut_off_frequency;
    FLOAT frequency_step;
    FLOAT notch_width;
};

struct ITPROGFILTBANK_nf_bank_entries
{
    LONG frequency;
    ULONG nf_parameters_arraylength;
    ITPROGFILTBANK_nf_bank_entries_nf_parameters nf_parameters[5];
};

struct ITPROGFILTBANK
{
    ULONG nf_bank_entries_arraylength;
    ITPROGFILTBANK_nf_bank_entries nf_bank_entries[24];
};

struct RANGECMP4
{
    ULONG range_data_arraylength;
    HEXBYTE range_data[16250];
};

struct ITPSDDETECT
{
    ULONG status_word;
    FLOAT frequency_start_m_hz;
    FLOAT step_size_hz;
    ULONG samples_arraylength;
    USHORT samples[1024];
};

struct ITDETECTSTATUS_interference_statuses
{
    LONG frequency;
    LONG int_detect_method;
    FLOAT parameter1;
    FLOAT parameter2;
    FLOAT parameter3;
    FLOAT parameter4;
    ULONG severity1;
    ULONG severity2;
    ULONG ITDETECTSTATUS_reserved3;
};

struct ITDETECTSTATUS
{
    ULONG interference_statuses_arraylength;
    ITDETECTSTATUS_interference_statuses interference_statuses[80];
};

struct NAVICRAWSUBFRAME
{
    ULONG sig_chan_num;
    ULONG sat_id;
    ULONG frame_id;
    HEXBYTE raw_subframe_data[33];
};

struct NAVICALMANAC
{
    ULONG w_na;
    DOUBLE ecc;
    ULONG toa;
    DOUBLE di;
    DOUBLE omega_dot;
    DOUBLE root_a;
    DOUBLE omega0;
    DOUBLE omega;
    DOUBLE m0;
    DOUBLE af0;
    DOUBLE af1;
    ULONG alm_svid;
    ULONG inter_sig_corr;
    ULONG spare;
    ULONG svid;
};

struct NAVICEPHEMERIS
{
    ULONG sat_id;
    ULONG wn;
    DOUBLE af0;
    DOUBLE af1;
    DOUBLE af2;
    ULONG ura;
    ULONG toc;
    DOUBLE tgd;
    DOUBLE delta_n;
    ULONG iodec;
    ULONG NAVICEPHEMERIS_reserved;
    ULONG l5_health;
    ULONG s_health;
    DOUBLE cuc;
    DOUBLE cus;
    DOUBLE cic;
    DOUBLE cis;
    DOUBLE crc;
    DOUBLE crs;
    DOUBLE i_dot;
    ULONG spare1;
    DOUBLE m0;
    ULONG toe;
    DOUBLE ecc;
    DOUBLE root_a;
    DOUBLE omega0;
    DOUBLE omega;
    DOUBLE omega_dot;
    DOUBLE i0;
    ULONG spare2;
    ULONG alert_flag;
    ULONG auto_nav_flag;
};

struct NAVICIONO
{
    ULONG prnid;
    DOUBLE alpha0;
    DOUBLE alpha1;
    DOUBLE alpha2;
    DOUBLE alpha3;
    DOUBLE beta0;
    DOUBLE beta1;
    DOUBLE beta2;
    DOUBLE beta3;
    ULONG spare;
};

struct NAVICSYSCLOCK
{
    ULONG prnid;
    DOUBLE a0_utc;
    DOUBLE a1_utc;
    DOUBLE a2_utc;
    LONG delta_tls;
    ULONG toutc;
    ULONG w_noutc;
    ULONG wnlsf;
    ULONG dn;
    LONG delta_tlsf;
    ULONG gnssid;
    DOUBLE a0;
    DOUBLE a1;
    DOUBLE a2;
    ULONG tot;
    ULONG w_not;
    ULONG spare;
};

struct RAWSBASFRAME2
{
    ULONG prn;
    ULONG signal_channel_num;
    UCHAR sbas_signal;
    UCHAR preamble_type;
    USHORT reserve;
    ULONG waas_msg_id;
    HEXBYTE raw_frame_data[29];
};

struct GALCNAVRAWPAGE
{
    ULONG chan;
    ULONG sat_id;
    ULONG page_id;
    HEXBYTE page[58];
};

struct QZSSCNAVRAWMESSAGE
{
    ULONG sig_chan_num;
    ULONG prn;
    LONG signal_type;
    ULONG message_id;
    HEXBYTE raw_frame_data[38];
};

struct GPSCNAVRAWMESSAGE
{
    ULONG sig_chan_num;
    ULONG prn;
    LONG signal_type;
    ULONG frame_id;
    HEXBYTE raw_frame_data[38];
};

struct SPRINKLERDATA
{
    ULONG config;
    ULONG start_sample_index;
    ULONG samples_arraylength;
    SHORT samples[1024];
};

struct SPRINKLERDATAH
{
    LONG signal;
    LONG freq_band;
    ULONG config;
    ULONG rf_start_frequency;
    ULONG sample_rate;
    LONG collection_status;
    USHORT adc_pulse_width;
    USHORT agc_pulse_modulus;
    ULONG num_samples_collected;
};

struct SKCALIBRATESTATUS_calibration_results
{
    LONG signal_type;
    LONG mode;
    LONG front_end_mode;
    LONG result;
    ULONG SKCALIBRATESTATUS_reserved1;
};

struct SKCALIBRATESTATUS
{
    LONG overall_status;
    LONG frequency_plan;
    ULONG calibration_results_arraylength;
    SKCALIBRATESTATUS_calibration_results calibration_results[10];
};

struct BDSBCNAV1EPHEMERIS
{
    ULONG satellite_id;
    ULONG wn;
    ULONG sat_status;
    ULONG iode;
    ULONG toe;
    ULONG sat_type;
    DOUBLE delta_a;
    DOUBLE a_dot;
    DOUBLE delta_n;
    DOUBLE n_dot;
    DOUBLE m0;
    DOUBLE eccentricity;
    DOUBLE omega;
    DOUBLE omega0;
    DOUBLE i0;
    DOUBLE omega_dot;
    DOUBLE i_dot;
    DOUBLE cis;
    DOUBLE cic;
    DOUBLE crs;
    DOUBLE crc;
    DOUBLE cus;
    DOUBLE cuc;
    ULONG iodc;
    ULONG toc;
    DOUBLE a0;
    DOUBLE a1;
    DOUBLE a2;
    DOUBLE tgdb1_cp;
    DOUBLE tgdb2_ap;
    DOUBLE iscb1_cd;
    ULONG BDSBCNAV1EPHEMERIS_reserved;
};

struct BDSBCNAV2EPHEMERIS
{
    ULONG satellite_id;
    ULONG wn;
    ULONG sat_status;
    ULONG iode;
    ULONG toe;
    ULONG sat_type;
    DOUBLE delta_a;
    DOUBLE a_dot;
    DOUBLE delta_n;
    DOUBLE n_dot;
    DOUBLE m0;
    DOUBLE eccentricity;
    DOUBLE omega;
    DOUBLE omega0;
    DOUBLE i0;
    DOUBLE omega_dot;
    DOUBLE i_dot;
    DOUBLE cis;
    DOUBLE cic;
    DOUBLE crs;
    DOUBLE crc;
    DOUBLE cus;
    DOUBLE cuc;
    ULONG iodc;
    ULONG toc;
    DOUBLE a0;
    DOUBLE a1;
    DOUBLE a2;
    DOUBLE tgdb1_cp;
    DOUBLE tgdb2_ap;
    DOUBLE iscb2ad;
    ULONG BDSBCNAV2EPHEMERIS_reserved;
};

struct BDSBCNAV1RAWMESSAGE
{
    ULONG sig_chan_num;
    ULONG prn;
    ULONG page_id;
    HEXBYTE subframe1_data[2];
    HEXBYTE subframe2_data[72];
    HEXBYTE subframe3_data[30];
};

struct BDSBCNAV2RAWMESSAGE
{
    ULONG sig_chan_num;
    ULONG prn;
    ULONG message_type;
    HEXBYTE raw_message_data[36];
    UCHAR BDSBCNAV2RAWMESSAGE_reserved1;
    UCHAR BDSBCNAV2RAWMESSAGE_reserved2;
    UCHAR BDSBCNAV2RAWMESSAGE_reserved3;
};

struct INSATT
{
    ULONG week;
    DOUBLE seconds;
    DOUBLE roll;
    DOUBLE pitch;
    DOUBLE azimuth;
    LONG ins_solution_status;
};

struct INSPOS
{
    ULONG week;
    DOUBLE seconds;
    DOUBLE latitude;
    DOUBLE longitude;
    DOUBLE height;
    LONG ins_solution_status;
};

struct INSSPD
{
    ULONG week;
    DOUBLE seconds;
    DOUBLE ground_track;
    DOUBLE horizontal_speed;
    DOUBLE up_vel;
    LONG ins_solution_status;
};

struct INSVEL
{
    ULONG week;
    DOUBLE seconds;
    DOUBLE north_vel;
    DOUBLE east_vel;
    DOUBLE up_vel;
    LONG ins_solution_status;
};

struct RAWIMU
{
    ULONG gps_week;
    DOUBLE gps_seconds;
    ULONG imu_status;
    LONG accel_z;
    LONG accel_y;
    LONG accel_x;
    LONG gyro_z;
    LONG gyro_y;
    LONG gyro_x;
};

struct INSATTS
{
    ULONG week;
    DOUBLE seconds;
    DOUBLE roll;
    DOUBLE pitch;
    DOUBLE azimuth;
    LONG ins_solution_status;
};

struct INSPOSS
{
    ULONG week;
    DOUBLE seconds;
    DOUBLE latitude;
    DOUBLE longitude;
    DOUBLE height;
    LONG ins_solution_status;
};

struct INSSPDS
{
    ULONG week;
    DOUBLE seconds;
    DOUBLE ground_track;
    DOUBLE horizontal_speed;
    DOUBLE up_vel;
    LONG ins_solution_status;
};

struct INSVELS
{
    ULONG week;
    DOUBLE seconds;
    DOUBLE north_vel;
    DOUBLE east_vel;
    DOUBLE up_vel;
    LONG ins_solution_status;
};

struct RAWIMUS
{
    ULONG gps_week;
    DOUBLE gps_seconds;
    ULONG imu_status;
    LONG accel_z;
    LONG accel_y;
    LONG accel_x;
    LONG gyro_z;
    LONG gyro_y;
    LONG gyro_x;
};

struct INSPVA
{
    ULONG week;
    DOUBLE seconds;
    DOUBLE latitude;
    DOUBLE longitude;
    DOUBLE height;
    DOUBLE north_vel;
    DOUBLE east_vel;
    DOUBLE up_vel;
    DOUBLE roll;
    DOUBLE pitch;
    DOUBLE azimuth;
    LONG ins_solution_status;
};

struct INSPVAS
{
    ULONG week;
    DOUBLE seconds;
    DOUBLE latitude;
    DOUBLE longitude;
    DOUBLE height;
    DOUBLE north_vel;
    DOUBLE east_vel;
    DOUBLE up_vel;
    DOUBLE roll;
    DOUBLE pitch;
    DOUBLE azimuth;
    LONG ins_solution_status;
};

struct CORRIMUDATA
{
    ULONG CORRIMUDATA_week;
    DOUBLE CORRIMUDATA_seconds;
    DOUBLE pitch_rate[3];
    DOUBLE roll_rate[3];
};

struct CORRIMUDATAS
{
    ULONG CORRIMUDATAS_week;
    DOUBLE CORRIMUDATAS_seconds;
    DOUBLE pitch_rate[3];
    DOUBLE roll_rate[3];
};

struct MARK1PVA
{
    ULONG week;
    DOUBLE seconds;
    DOUBLE latitude;
    DOUBLE longitude;
    DOUBLE height;
    DOUBLE north_vel;
    DOUBLE east_vel;
    DOUBLE up_vel;
    DOUBLE roll;
    DOUBLE pitch;
    DOUBLE azimuth;
    LONG ins_solution_status;
};

struct MARK2PVA
{
    ULONG week;
    DOUBLE seconds;
    DOUBLE latitude;
    DOUBLE longitude;
    DOUBLE height;
    DOUBLE north_vel;
    DOUBLE east_vel;
    DOUBLE up_vel;
    DOUBLE roll;
    DOUBLE pitch;
    DOUBLE azimuth;
    LONG ins_solution_status;
};

struct MARK3PVA
{
    ULONG week;
    DOUBLE seconds;
    DOUBLE latitude;
    DOUBLE longitude;
    DOUBLE height;
    DOUBLE north_vel;
    DOUBLE east_vel;
    DOUBLE up_vel;
    DOUBLE roll;
    DOUBLE pitch;
    DOUBLE azimuth;
    LONG ins_solution_status;
};

struct MARK4PVA
{
    ULONG week;
    DOUBLE seconds;
    DOUBLE latitude;
    DOUBLE longitude;
    DOUBLE height;
    DOUBLE north_vel;
    DOUBLE east_vel;
    DOUBLE up_vel;
    DOUBLE roll;
    DOUBLE pitch;
    DOUBLE azimuth;
    LONG ins_solution_status;
};

struct TILTDATA
{
    ULONG sensor_status;
    DOUBLE x_acceleration;
    DOUBLE y_acceleration;
    DOUBLE TILTDATA_reserved1;
    DOUBLE pitch;
    DOUBLE roll;
    DOUBLE TILTDATA_reserved2;
    ULONG TILTDATA_reserved3;
};

struct TAGGEDMARK1PVA
{
    ULONG week;
    DOUBLE seconds;
    DOUBLE latitude;
    DOUBLE longitude;
    DOUBLE height;
    DOUBLE north_vel;
    DOUBLE east_vel;
    DOUBLE up_vel;
    DOUBLE roll;
    DOUBLE pitch;
    DOUBLE azimuth;
    LONG ins_solution_status;
    ULONG tag_id;
};

struct TAGGEDMARK2PVA
{
    ULONG week;
    DOUBLE seconds;
    DOUBLE latitude;
    DOUBLE longitude;
    DOUBLE height;
    DOUBLE north_vel;
    DOUBLE east_vel;
    DOUBLE up_vel;
    DOUBLE roll;
    DOUBLE pitch;
    DOUBLE azimuth;
    LONG ins_solution_status;
    ULONG tag_id;
};

struct IMURATEPVAS
{
    ULONG week;
    DOUBLE seconds;
    DOUBLE latitude;
    DOUBLE longitude;
    DOUBLE height;
    DOUBLE north_vel;
    DOUBLE east_vel;
    DOUBLE up_vel;
    DOUBLE roll;
    DOUBLE pitch;
    DOUBLE azimuth;
    LONG ins_solution_status;
};

struct VARIABLELEVERARM
{
    DOUBLE offsets[3];
    DOUBLE stdevs[3];
};

struct GIMBALLEDPVA
{
    ULONG week;
    DOUBLE seconds;
    DOUBLE latitude;
    DOUBLE longitude;
    DOUBLE height;
    DOUBLE north_vel;
    DOUBLE east_vel;
    DOUBLE up_vel;
    DOUBLE roll;
    DOUBLE pitch;
    DOUBLE azimuth;
    LONG ins_solution_status;
};

struct TAGGEDMARK3PVA
{
    ULONG week;
    DOUBLE seconds;
    DOUBLE latitude;
    DOUBLE longitude;
    DOUBLE height;
    DOUBLE north_vel;
    DOUBLE east_vel;
    DOUBLE up_vel;
    DOUBLE roll;
    DOUBLE pitch;
    DOUBLE azimuth;
    LONG ins_solution_status;
    ULONG tag_id;
};

struct TAGGEDMARK4PVA
{
    ULONG week;
    DOUBLE seconds;
    DOUBLE latitude;
    DOUBLE longitude;
    DOUBLE height;
    DOUBLE north_vel;
    DOUBLE east_vel;
    DOUBLE up_vel;
    DOUBLE roll;
    DOUBLE pitch;
    DOUBLE azimuth;
    LONG ins_solution_status;
    ULONG tag_id;
};

struct IMURATECORRIMUS
{
    ULONG IMURATECORRIMUS_week;
    DOUBLE IMURATECORRIMUS_seconds;
    DOUBLE pitch;
    DOUBLE roll;
    DOUBLE azimuth;
    DOUBLE x_acceleration;
    DOUBLE y_acceleration;
    DOUBLE z_acceleration;
};

struct HEAVE
{
    ULONG week;
    DOUBLE seconds;
    DOUBLE heave;
};

struct RELINSPVA
{
    LONG rel_ins_output;
    DOUBLE delta_pos_n;
    DOUBLE delta_pos_e;
    DOUBLE delta_pos_u;
    DOUBLE delta_vel_n;
    DOUBLE delta_vel_e;
    DOUBLE delta_vel_u;
    DOUBLE delta_roll;
    DOUBLE delta_pitch;
    DOUBLE delta_heading;
    FLOAT diff_age;
    CHAR rover_id[4];
    LONG rover_ins_status;
    CHAR master_id[4];
    LONG master_ins_status;
    LONG rtk_baseline_status;
    ULONG extended_sol_stat;
};

struct TSS1
{
    LONG ins_solution_status;
    DOUBLE x;
    DOUBLE y;
    DOUBLE z;
    DOUBLE heave;
    DOUBLE roll;
    DOUBLE pitch;
    ULONG week;
    DOUBLE seconds;
};

struct INSATTX
{
    LONG ins_solution_status;
    LONG position_type;
    DOUBLE roll;
    DOUBLE pitch;
    DOUBLE azimuth;
    FLOAT roll_std_dev;
    FLOAT pitch_std_dev;
    FLOAT azimuth_std_dev;
    ULONG extended_sol_stat;
    USHORT time_since_update;
};

struct INSVELX
{
    LONG ins_solution_status;
    LONG position_type;
    DOUBLE north_vel;
    DOUBLE east_vel;
    DOUBLE up_vel;
    FLOAT north_vel_std_dev;
    FLOAT east_vel_std_dev;
    FLOAT up_vel_std_dev;
    ULONG extended_sol_stat;
    USHORT time_since_update;
};

struct INSPOSX
{
    LONG ins_solution_status;
    LONG position_type;
    DOUBLE latitude;
    DOUBLE longitude;
    DOUBLE orthometric_height;
    FLOAT undulation;
    FLOAT latitude_std_dev;
    FLOAT longitude_std_dev;
    FLOAT height_std_dev;
    ULONG extended_sol_stat;
    USHORT time_since_update;
};

struct RAWIMUX
{
    UCHAR imu_status_info;
    UCHAR imu_type;
    USHORT gps_week;
    DOUBLE gps_seconds;
    ULONG imu_status;
    LONG accel_z;
    LONG accel_y;
    LONG accel_x;
    LONG gyro_z;
    LONG gyro_y;
    LONG gyro_x;
};

struct RAWIMUSX
{
    UCHAR imu_info;
    UCHAR imu_type;
    USHORT gnss_week;
    DOUBLE gnss_week_seconds;
    ULONG imu_status;
    LONG z_accel;
    LONG neg_y_accel;
    LONG x_accel;
    LONG z_gyro;
    LONG neg_y_gyro;
    LONG x_gyro;
};

struct INSPVAX
{
    LONG ins_solution_status;
    LONG position_type;
    DOUBLE latitude;
    DOUBLE longitude;
    DOUBLE height;
    FLOAT undulation;
    DOUBLE north_vel;
    DOUBLE east_vel;
    DOUBLE up_vel;
    DOUBLE roll;
    DOUBLE pitch;
    DOUBLE azimuth;
    FLOAT latitude_std_dev;
    FLOAT longitude_std_dev;
    FLOAT height_std_dev;
    FLOAT north_vel_std_dev;
    FLOAT east_vel_std_dev;
    FLOAT up_vel_std_dev;
    FLOAT roll_std_dev;
    FLOAT pitch_std_dev;
    FLOAT azimuth_std_dev;
    ULONG ext_sol_stat;
    USHORT time_since_update;
};

struct SYNCHEAVE
{
    DOUBLE heave;
    DOUBLE heave_std_dev;
};

struct DELAYEDHEAVE
{
    DOUBLE heave;
    DOUBLE heave_std_dev;
};

struct SYNCRELINSPVA
{
    LONG rel_ins_output;
    DOUBLE delta_pos_n;
    DOUBLE delta_pos_e;
    DOUBLE delta_pos_u;
    DOUBLE delta_vel_n;
    DOUBLE delta_vel_e;
    DOUBLE delta_vel_u;
    DOUBLE delta_roll;
    DOUBLE delta_pitch;
    DOUBLE delta_heading;
    FLOAT diff_age;
    CHAR rover_id[4];
    LONG rover_ins_status;
    CHAR master_id[4];
    LONG master_ins_status;
    LONG rtk_baseline_status;
    ULONG extended_sol_stat;
};

struct IMURATEPVA
{
    ULONG week;
    DOUBLE seconds;
    DOUBLE latitude;
    DOUBLE longitude;
    DOUBLE height;
    DOUBLE north_vel;
    DOUBLE east_vel;
    DOUBLE up_vel;
    DOUBLE roll;
    DOUBLE pitch;
    DOUBLE azimuth;
    LONG ins_solution_status;
};

struct INSUPDATESTATUS
{
    LONG gnss_position_type;
    INT num_psr_obs;
    INT num_adr_obs;
    INT num_dop_obs;
    LONG DMI_update_status;
    LONG heading_update_status;
    ULONG ext_sol_stat;
    ULONG ins_update_options;
    ULONG INSUPDATESTATUS_reserved1;
    ULONG INSUPDATESTATUS_reserved2;
};

struct INSPVACMP
{
    ULONG week_milliseconds;
    UCHAR ins_solution_status;
    UCHAR gnss_position_type;
    LONGLONG latitude;
    LONGLONG longitude;
    LONG height;
    SHORT north_vel;
    SHORT east_vel;
    SHORT up_vel;
    SHORT roll;
    SHORT pitch;
    USHORT azimuth;
    SHORT azimuth_rate;
    ULONG milliseconds;
};

struct INSPVASDCMP
{
    USHORT INSPVASDCMP_week;
    ULONG INSPVASDCMP_milliseconds;
    USHORT latitude_std_dev;
    USHORT longitude_std_dev;
    USHORT height_std_dev;
    USHORT north_vel_std_dev;
    USHORT east_vel_std_dev;
    USHORT up_vel_std_dev;
    USHORT roll_std_dev;
    USHORT pitch_std_dev;
    USHORT azimuth_std_dev;
    UCHAR time_since_update;
    UCHAR position_type;
    ULONG extended_sol_stat;
    UCHAR align_age;
    ULONG milliseconds;
};

struct INSCONFIG_ins_translations
{
    LONG ins_offset;
    LONG input_frame;
    FLOAT offset[3];
    FLOAT offset_stdev[3];
    LONG source_status;
};

struct INSCONFIG_ins_rotations
{
    LONG ins_offset_26;
    LONG input_frame_27;
    FLOAT offset_28[3];
    FLOAT offset_stdev_29[3];
    LONG source_status_30;
};

struct INSCONFIG
{
    LONG imu_type;
    UCHAR mapping;
    UCHAR scaled_alignment_vel;
    USHORT heave_window;
    LONG ins_profile;
    ULONG enabled_updates;
    LONG alignment_mode;
    LONG rel_ins_output;
    BOOL rel_from_master;
    ULONG ins_rx_status;
    UCHAR ins_seed;
    UCHAR initial_ins_state;
    USHORT ins_grade_mode;
    ULONG INSCONFIG_reserved2;
    ULONG INSCONFIG_reserved3;
    ULONG INSCONFIG_reserved4;
    ULONG INSCONFIG_reserved5;
    ULONG INSCONFIG_reserved6;
    ULONG INSCONFIG_reserved7;
    ULONG ins_translations_arraylength;
    INSCONFIG_ins_translations ins_translations[11];
    ULONG ins_rotations_arraylength;
    INSCONFIG_ins_rotations ins_rotations[9];
};

struct INSCALSTATUS
{
    LONG offset_type;
    FLOAT x_offset[3];
    FLOAT y_offset[3];
    LONG z_offset;
    ULONG x_std_dev;
};

struct INSSTDEV
{
    FLOAT latitude_std_dev;
    FLOAT longitude_std_dev;
    FLOAT height_std_dev;
    FLOAT north_vel_std_dev;
    FLOAT east_vel_std_dev;
    FLOAT up_vel_std_dev;
    FLOAT roll_std_dev;
    FLOAT pitch_std_dev;
    FLOAT azimuth_std_dev;
    ULONG extended_sol_stat;
    USHORT time_since_update;
    USHORT ushort;
    ULONG enabled_updates;
    ULONG ulong;
};

struct INSSTDEVS
{
    FLOAT latitude_std_dev;
    FLOAT longitude_std_dev;
    FLOAT height_std_dev;
    FLOAT north_vel_std_dev;
    FLOAT east_vel_std_dev;
    FLOAT up_vel_std_dev;
    FLOAT roll_std_dev;
    FLOAT pitch_std_dev;
    FLOAT azimuth_std_dev;
    ULONG extended_sol_stat;
    USHORT time_since_update;
    USHORT ushort;
    ULONG enabled_updates;
    ULONG ulong;
};

struct INSATTQS
{
    ULONG INSATTQS_week;
    DOUBLE INSATTQS_seconds;
    DOUBLE quaternion_w[4];
    LONG quaternion_x;
};

struct INSSEEDSTATUS
{
    LONG injection_status;
    LONG validity_status;
    FLOAT pitch[3];
    DOUBLE roll[3];
    FLOAT azimuth;
    ULONG position_x;
    ULONG position_y;
    ULONG position_z;
    ULONG undulation;
};

struct CORRIMUS
{
    ULONG accum_count;
    DOUBLE corr_wb_ib[3];
    DOUBLE corr_fb[3];
    FLOAT latency_ms;
    ULONG ulong;
};

struct TILTSTATUS
{
    ULONG compensation_status;
    DOUBLE pitch;
    DOUBLE roll;
    DOUBLE x;
    DOUBLE y;
    DOUBLE z;
    DOUBLE azimuth;
    ULONG ulong;
};

struct INSVELUSER
{
    LONG ins_solution_status;
    DOUBLE north_vel;
    DOUBLE east_vel;
    DOUBLE up_vel;
    FLOAT north_vel_std_dev;
    FLOAT east_vel_std_dev;
    FLOAT up_vel_std_dev;
    FLOAT slip_angle;
    ULONG ulong;
    ULONG ulong_9;
    ULONG ext_vel_status;
};

struct INSDATUMINFO
{
    CHAR name[32];
    ULONG epsg_code;
    DOUBLE epoch;
    LONG transformation_status;
    FLOAT INSDATUMINFO_reserved;
    ULONG INSDATUMINFO_reserved_5;
};

struct CLOCKMODEL
{
    LONG status;
    ULONG reject_count;
    ULONG propagation_time;
    ULONG update_time;
    DOUBLE bias[3];
    DOUBLE rate[9];
    DOUBLE CLOCKMODEL_reserved1;
    DOUBLE bias_variance;
    BOOL covariance;
};

struct BESTPOS
{
    LONG solution_status;
    LONG position_type;
    DOUBLE latitude;
    DOUBLE longitude;
    DOUBLE orthometric_height;
    FLOAT undulation;
    LONG datum_id;
    FLOAT latitude_std_dev;
    FLOAT longitude_std_dev;
    FLOAT height_std_dev;
    CHAR base_id[4];
    FLOAT diff_age;
    FLOAT solution_age;
    UCHAR num_svs;
    UCHAR num_soln_svs;
    UCHAR num_soln_L1_svs;
    UCHAR num_soln_multi_svs;
    UCHAR extended_solution_status2;
    UCHAR ext_sol_stat;
    UCHAR gal_and_bds_mask;
    UCHAR gps_and_glo_mask;
};

struct PSRPOS
{
    LONG solution_status;
    LONG position_type;
    DOUBLE latitude;
    DOUBLE longitude;
    DOUBLE orthometric_height;
    FLOAT undulation;
    LONG datum_id;
    FLOAT latitude_std_dev;
    FLOAT longitude_std_dev;
    FLOAT height_std_dev;
    CHAR base_id[4];
    FLOAT diff_age;
    FLOAT solution_age;
    UCHAR num_svs;
    UCHAR num_soln_svs;
    UCHAR char_as_int;
    UCHAR char_as_int_16;
    UCHAR extended_solution_status2;
    UCHAR ext_sol_stat;
    UCHAR gal_and_bds_mask;
    UCHAR gps_and_glo_mask;
};

struct MATCHEDPOS
{
    LONG solution_status;
    LONG position_type;
    DOUBLE latitude;
    DOUBLE longitude;
    DOUBLE orthometric_height;
    FLOAT undulation;
    LONG datum_id;
    FLOAT latitude_std_dev;
    FLOAT longitude_std_dev;
    FLOAT height_std_dev;
    CHAR base_id[4];
    FLOAT diff_age;
    FLOAT solution_age;
    UCHAR num_svs;
    UCHAR num_soln_svs;
    UCHAR num_soln_L1_svs;
    UCHAR num_soln_multi_svs;
    UCHAR extended_solution_status2;
    UCHAR ext_sol_stat;
    UCHAR gal_and_bds_mask;
    UCHAR gps_and_glo_mask;
};

struct BESTVEL
{
    LONG solution_status;
    LONG velocity_type;
    FLOAT latency;
    FLOAT diff_age;
    DOUBLE horizontal_speed;
    DOUBLE ground_track;
    DOUBLE vertical_speed;
    LONG BESTVEL_reserved1;
};

struct PSRVEL
{
    LONG solution_status;
    LONG velocity_type;
    FLOAT latency;
    FLOAT diff_age;
    DOUBLE horizontal_speed;
    DOUBLE ground_track;
    DOUBLE vertical_speed;
    LONG PSRVEL_reserved1;
};

struct RTKPOS
{
    LONG solution_status;
    LONG position_type;
    DOUBLE latitude;
    DOUBLE longitude;
    DOUBLE orthometric_height;
    FLOAT undulation;
    LONG datum_id;
    FLOAT latitude_std_dev;
    FLOAT longitude_std_dev;
    FLOAT height_std_dev;
    CHAR base_id[4];
    FLOAT diff_age;
    FLOAT solution_age;
    UCHAR num_svs;
    UCHAR num_soln_svs;
    UCHAR num_soln_L1_svs;
    UCHAR num_soln_multi_svs;
    UCHAR extended_solution_status2;
    UCHAR ext_sol_stat;
    UCHAR gal_and_bds_mask;
    UCHAR gps_and_glo_mask;
};

struct NAVIGATE
{
    LONG solution_status;
    LONG position_type;
    LONG velocity_status;
    LONG nav_status;
    DOUBLE distance;
    DOUBLE bearing;
    DOUBLE along_track;
    DOUBLE x_track;
    ULONG eta_weeks;
    DOUBLE eta_seconds;
};

struct AVEPOS
{
    DOUBLE latitude;
    DOUBLE longitude;
    DOUBLE orthometric_height;
    FLOAT latitude_std_dev;
    FLOAT longitude_std_dev;
    FLOAT height_std_dev;
    LONG ave_status;
    ULONG ave_time;
    ULONG num_sample;
};

struct PSRDOP
{
    FLOAT gdop;
    FLOAT pdop;
    FLOAT hdop;
    FLOAT htdop;
    FLOAT tdop;
    FLOAT gps_elev_mask;
    ULONG sats_arraylength;
    ULONG sats[325];
};

struct REFSTATION
{
    ULONG ref_status;
    DOUBLE ecef_x;
    DOUBLE ecef_y;
    DOUBLE ecef_z;
    ULONG health;
    LONG ref_type;
    UCHAR ref_id[5];
};

struct MARKPOS
{
    LONG solution_status;
    LONG position_type;
    DOUBLE latitude;
    DOUBLE longitude;
    DOUBLE orthometric_height;
    FLOAT undulation;
    LONG datum_id;
    FLOAT latitude_std_dev;
    FLOAT longitude_std_dev;
    FLOAT height_std_dev;
    CHAR base_id[4];
    FLOAT diff_age;
    FLOAT solution_age;
    UCHAR num_svs;
    UCHAR num_soln_svs;
    UCHAR num_soln_L1_svs;
    UCHAR num_soln_multi_svs;
    UCHAR char_as_int;
    UCHAR ext_sol_stat;
    UCHAR gal_and_bds_mask;
    UCHAR gps_and_glo_mask;
};

struct RTKVEL
{
    LONG solution_status;
    LONG velocity_type;
    FLOAT latency;
    FLOAT diff_age;
    DOUBLE horizontal_speed;
    DOUBLE ground_track;
    DOUBLE vertical_speed;
    LONG RTKVEL_reserved1;
};

struct BESTXYZ
{
    LONG p_solution_status;
    LONG position_type;
    DOUBLE p_x;
    DOUBLE p_y;
    DOUBLE p_z;
    FLOAT p_x_st_dev;
    FLOAT p_y_st_dev;
    FLOAT p_z_st_dev;
    LONG v_solution_status;
    LONG velocity_type;
    DOUBLE v_x;
    DOUBLE v_y;
    DOUBLE v_z;
    FLOAT v_x_st_dev;
    FLOAT v_y_st_dev;
    FLOAT v_z_st_dev;
    CHAR station_id[4];
    FLOAT v_latency;
    FLOAT diff_age;
    FLOAT sol_age;
    UCHAR num_svs;
    UCHAR num_soln_svs;
    UCHAR num_ggL1;
    UCHAR num_soln_multi_svs;
    UCHAR BESTXYZ_reserved1;
    UCHAR ext_sol_stat;
    UCHAR gal_and_bds_mask;
    UCHAR gps_and_glo_mask;
};

struct MATCHEDXYZ
{
    LONG solution_status;
    LONG position_type;
    DOUBLE x;
    DOUBLE y;
    DOUBLE z;
    FLOAT x_std_dev;
    FLOAT y_std_dev;
    FLOAT z_std_dev;
    CHAR base_id[4];
    UCHAR num_svs;
    UCHAR num_soln_svs;
    UCHAR num_soln_L1_svs;
    UCHAR num_soln_multi_svs;
    UCHAR char_as_int;
    UCHAR ext_sol_stat;
    UCHAR gal_and_bds_mask;
    UCHAR gps_and_glo_mask;
};

struct PSRXYZ
{
    LONG p_solution_status;
    LONG position_type;
    DOUBLE p_x;
    DOUBLE p_y;
    DOUBLE p_z;
    FLOAT p_x_st_dev;
    FLOAT p_y_st_dev;
    FLOAT p_z_st_dev;
    LONG v_solution_status;
    LONG velocity_type;
    DOUBLE v_x;
    DOUBLE v_y;
    DOUBLE v_z;
    FLOAT v_x_st_dev;
    FLOAT v_y_st_dev;
    FLOAT v_z_st_dev;
    CHAR station_id[4];
    FLOAT v_latency;
    FLOAT diff_age;
    FLOAT sol_age;
    UCHAR num_svs;
    UCHAR num_soln_svs;
    UCHAR num_ggL1;
    UCHAR num_soln_multi_svs;
    UCHAR PSRXYZ_reserved1;
    UCHAR ext_sol_stat;
    UCHAR gal_and_bds_mask;
    UCHAR gps_and_glo_mask;
};

struct RTKXYZ
{
    LONG p_solution_status;
    LONG position_type;
    DOUBLE p_x;
    DOUBLE p_y;
    DOUBLE p_z;
    FLOAT p_x_st_dev;
    FLOAT p_y_st_dev;
    FLOAT p_z_st_dev;
    LONG v_solution_status;
    LONG velocity_type;
    DOUBLE v_x;
    DOUBLE v_y;
    DOUBLE v_z;
    FLOAT v_x_st_dev;
    FLOAT v_y_st_dev;
    FLOAT v_z_st_dev;
    CHAR station_id[4];
    FLOAT v_latency;
    FLOAT diff_age;
    FLOAT sol_age;
    UCHAR num_svs;
    UCHAR num_soln_svs;
    UCHAR num_ggL1;
    UCHAR num_soln_multi_svs;
    UCHAR RTKXYZ_reserved1;
    UCHAR ext_sol_stat;
    UCHAR gal_and_bds_mask;
    UCHAR gps_and_glo_mask;
};

struct PDPPOS
{
    LONG solution_status;
    LONG position_type;
    DOUBLE latitude;
    DOUBLE longitude;
    DOUBLE orthometric_height;
    FLOAT undulation;
    LONG datum_id;
    FLOAT latitude_std_dev;
    FLOAT longitude_std_dev;
    FLOAT height_std_dev;
    CHAR base_id[4];
    FLOAT diff_age;
    FLOAT solution_age;
    UCHAR num_svs;
    UCHAR num_soln_svs;
    UCHAR char_as_int;
    UCHAR char_as_int_16;
    UCHAR extended_solution_status2;
    UCHAR ext_sol_stat;
    UCHAR gal_and_bds_mask;
    UCHAR gps_and_glo_mask;
};

struct PDPVEL
{
    LONG solution_status;
    LONG velocity_type;
    FLOAT latency;
    FLOAT diff_age;
    DOUBLE horizontal_speed;
    DOUBLE ground_track;
    DOUBLE vertical_speed;
    LONG PDPVEL_reserved1;
};

struct PDPXYZ
{
    LONG p_solution_status;
    LONG position_type;
    DOUBLE p_x;
    DOUBLE p_y;
    DOUBLE p_z;
    FLOAT p_x_st_dev;
    FLOAT p_y_st_dev;
    FLOAT p_z_st_dev;
    LONG v_solution_status;
    LONG velocity_type;
    DOUBLE v_x;
    DOUBLE v_y;
    DOUBLE v_z;
    FLOAT v_x_st_dev;
    FLOAT v_y_st_dev;
    FLOAT v_z_st_dev;
    CHAR station_id[4];
    FLOAT v_latency;
    FLOAT diff_age;
    FLOAT sol_age;
    UCHAR num_svs;
    UCHAR num_soln_svs;
    UCHAR num_ggL1;
    UCHAR num_soln_multi_svs;
    UCHAR PDPXYZ_reserved1;
    UCHAR ext_sol_stat;
    UCHAR gal_and_bds_mask;
    UCHAR gps_and_glo_mask;
};

struct MARK2POS
{
    LONG solution_status;
    LONG position_type;
    DOUBLE latitude;
    DOUBLE longitude;
    DOUBLE orthometric_height;
    FLOAT undulation;
    LONG datum_id;
    FLOAT latitude_std_dev;
    FLOAT longitude_std_dev;
    FLOAT height_std_dev;
    CHAR base_id[4];
    FLOAT diff_age;
    FLOAT solution_age;
    UCHAR num_svs;
    UCHAR num_soln_svs;
    UCHAR num_soln_L1_svs;
    UCHAR num_soln_multi_svs;
    UCHAR char_as_int;
    UCHAR ext_sol_stat;
    UCHAR gal_and_bds_mask;
    UCHAR gps_and_glo_mask;
};

struct MARK2TIME
{
    LONG week;
    DOUBLE seconds;
    DOUBLE offset;
    DOUBLE offset_std;
    DOUBLE utc_offset;
    LONG status;
};

struct BSLNXYZ
{
    LONG solution_status;
    LONG bsln_type;
    DOUBLE b_x;
    DOUBLE b_y;
    DOUBLE b_z;
    FLOAT b_x_st_dev;
    FLOAT b_y_st_dev;
    FLOAT b_z_st_dev;
    CHAR station_id[4];
    UCHAR v_latency;
    UCHAR diff_age;
    UCHAR sol_age;
    UCHAR num_svs;
    UCHAR num_soln_svs;
    UCHAR num_ggL1;
    UCHAR num_soln_multi_svs;
    UCHAR BSLNXYZ_reserved1;
};

struct BESTUTM
{
    LONG solution_status;
    LONG position_type;
    ULONG zone_number;
    ULONG zone_letter;
    DOUBLE northing;
    DOUBLE easting;
    DOUBLE height;
    FLOAT undulation;
    LONG datum_id;
    FLOAT northing_st_dev;
    FLOAT easting_st_dev;
    FLOAT height_st_dev;
    CHAR base_id[4];
    FLOAT diff_age;
    FLOAT solution_age;
    UCHAR num_svs;
    UCHAR num_soln_svs;
    UCHAR num_soln_L1_svs;
    UCHAR num_soln_multi_svs;
    UCHAR char_as_int;
    UCHAR ext_sol_stat;
    UCHAR gal_and_bds_mask;
    UCHAR gps_and_glo_mask;
};

struct RTKDOP
{
    FLOAT gdop;
    FLOAT pdop;
    FLOAT hdop;
    FLOAT htdop;
    FLOAT tdop;
    FLOAT gps_elev_mask;
    ULONG sats_arraylength;
    ULONG sats[325];
};

struct MASTERPOS
{
    LONG solution_status;
    LONG position_type;
    DOUBLE latitude;
    DOUBLE longitude;
    DOUBLE orthometric_height;
    FLOAT undulation;
    LONG datum_id;
    FLOAT latitude_std_dev;
    FLOAT longitude_std_dev;
    FLOAT height_std_dev;
    CHAR base_id[4];
    FLOAT float;
    FLOAT float_12;
    UCHAR num_svs;
    UCHAR num_soln_svs;
    UCHAR num_soln_L1_svs;
    UCHAR num_soln_multi_svs;
    UCHAR extended_solution_status2;
    UCHAR char_as_int;
    UCHAR char_as_int_19;
    UCHAR char_as_int_20;
};

struct ROVERPOS
{
    LONG solution_status;
    LONG position_type;
    DOUBLE latitude;
    DOUBLE longitude;
    DOUBLE orthometric_height;
    FLOAT undulation;
    LONG datum_id;
    FLOAT latitude_std_dev;
    FLOAT longitude_std_dev;
    FLOAT height_std_dev;
    CHAR base_id[4];
    FLOAT float;
    FLOAT float_12;
    UCHAR num_svs;
    UCHAR num_soln_svs;
    UCHAR num_soln_L1_svs;
    UCHAR num_soln_multi_svs;
    UCHAR extended_solution_status2;
    UCHAR char_as_int;
    UCHAR char_as_int_19;
    UCHAR char_as_int_20;
};

struct PSRSATS_satellite_entries
{
    LONG system_type;
    SATELLITEID id;
    LONG status;
    ULONG status_mask;
};

struct PSRSATS
{
    ULONG satellite_entries_arraylength;
    PSRSATS_satellite_entries satellite_entries[325];
};

struct PSRDOP2_tdo_ps
{
    LONG system;
    FLOAT dop;
};

struct PSRDOP2
{
    FLOAT gdop;
    FLOAT pdop;
    FLOAT hdop;
    FLOAT vdop;
    ULONG tdo_ps_arraylength;
    PSRDOP2_tdo_ps tdo_ps[5];
};

struct RTKDOP2_tdo_ps
{
    LONG system;
    FLOAT dop;
};

struct RTKDOP2
{
    FLOAT gdop;
    FLOAT pdop;
    FLOAT hdop;
    FLOAT vdop;
    ULONG tdo_ps_arraylength;
    RTKDOP2_tdo_ps tdo_ps[5];
};

struct RTKSATS_satellite_entries
{
    LONG system_type;
    SATELLITEID id;
    LONG status;
    ULONG status_mask;
};

struct RTKSATS
{
    ULONG satellite_entries_arraylength;
    RTKSATS_satellite_entries satellite_entries[325];
};

struct MATCHEDSATS_satellite_entries
{
    LONG system_type;
    SATELLITEID id;
    LONG status;
    ULONG status_mask;
};

struct MATCHEDSATS
{
    ULONG satellite_entries_arraylength;
    MATCHEDSATS_satellite_entries satellite_entries[325];
};

struct BESTSATS_satellite_entries
{
    LONG system_type;
    SATELLITEID id;
    LONG status;
    ULONG status_mask;
};

struct BESTSATS
{
    ULONG satellite_entries_arraylength;
    BESTSATS_satellite_entries satellite_entries[325];
};

struct PDPSATS_satellite_entries
{
    LONG system_type;
    SATELLITEID id;
    LONG status;
    ULONG status_mask;
};

struct PDPSATS
{
    ULONG satellite_entries_arraylength;
    PDPSATS_satellite_entries satellite_entries[325];
};

struct RAIMSTATUS_rejected_s_vs
{
    LONG system_type;
    SATELLITEID id;
};

struct RAIMSTATUS
{
    LONG mode;
    LONG integrity_status;
    LONG hpl_status;
    DOUBLE hpl;
    LONG vpl_status;
    DOUBLE vpl;
    ULONG rejected_s_vs_arraylength;
    RAIMSTATUS_rejected_s_vs rejected_s_vs[20];
};

struct ALIGNBSLNXYZ
{
    LONG solution_status;
    LONG position_type;
    DOUBLE x;
    DOUBLE y;
    DOUBLE z;
    FLOAT x_st_dev;
    FLOAT y_st_dev;
    FLOAT z_st_dev;
    CHAR rover_id[4];
    CHAR master_id[4];
    UCHAR num_svs;
    UCHAR num_soln_svs;
    UCHAR num_obs;
    UCHAR num_multi;
    UCHAR ALIGNBSLNXYZ_reserved1;
    UCHAR ext_sol_stat;
    UCHAR gal_and_bds_mask;
    UCHAR gps_and_glo_mask;
};

struct ALIGNBSLNENU
{
    LONG solution_status;
    LONG position_type;
    DOUBLE dx;
    DOUBLE dy;
    DOUBLE dz;
    FLOAT dx_st_dev;
    FLOAT dy_st_dev;
    FLOAT dz_st_dev;
    CHAR rover_id[4];
    CHAR master_id[4];
    UCHAR num_svs;
    UCHAR num_soln_svs;
    UCHAR num_obs;
    UCHAR num_multi;
    UCHAR ALIGNBSLNENU_reserved1;
    UCHAR ext_sol_stat;
    UCHAR gal_and_bds_mask;
    UCHAR gps_and_glo_mask;
};

struct HEADINGSATS_satellite_entries
{
    LONG system_type;
    SATELLITEID id;
    LONG status;
    ULONG status_mask;
};

struct HEADINGSATS
{
    ULONG satellite_entries_arraylength;
    HEADINGSATS_satellite_entries satellite_entries[325];
};

struct REFSTATIONINFO
{
    DOUBLE latitude;
    DOUBLE longitude;
    DOUBLE height;
    LONG datum;
    FLOAT arp_height;
    ULONG health;
    LONG ref_type;
    UCHAR station_id[5];
    UCHAR antenna_model[32];
    UCHAR antenna_serial[32];
};

struct ALIGNDOP
{
    FLOAT gdop;
    FLOAT pdop;
    FLOAT hdop;
    FLOAT htdop;
    FLOAT tdop;
    FLOAT gps_elev_mask;
    ULONG sats_arraylength;
    ULONG sats[325];
};

struct HEADING2
{
    LONG solution_status;
    LONG position_type;
    FLOAT b_length;
    FLOAT heading;
    FLOAT pitch;
    FLOAT float;
    FLOAT heading_std_dev;
    FLOAT pitch_std_dev;
    CHAR rover_id[4];
    CHAR base_id[4];
    UCHAR num_svs;
    UCHAR num_soln_svs;
    UCHAR num_soln_L1_svs;
    UCHAR num_soln_multi_svs;
    UCHAR extended_solution_status2;
    UCHAR ext_sol_stat;
    UCHAR gal_and_bds_mask;
    UCHAR gps_and_glo_mask;
};

struct SBASALMANAC
{
    ULONG satellite_id;
    LONG system_variant;
    ULONG t0;
    USHORT data_id;
    USHORT health;
    LONG x;
    LONG y;
    LONG z;
    LONG x_vel;
    LONG y_vel;
    LONG z_vel;
};

struct BESTGNSSPOS
{
    LONG solution_status;
    LONG position_type;
    DOUBLE latitude;
    DOUBLE longitude;
    DOUBLE orthometric_height;
    FLOAT undulation;
    LONG datum_id;
    FLOAT latitude_std_dev;
    FLOAT longitude_std_dev;
    FLOAT height_std_dev;
    CHAR base_id[4];
    FLOAT diff_age;
    FLOAT solution_age;
    UCHAR num_svs;
    UCHAR num_soln_svs;
    UCHAR num_soln_L1_svs;
    UCHAR num_soln_multi_svs;
    UCHAR extended_solution_status2;
    UCHAR ext_sol_stat;
    UCHAR gal_and_bds_mask;
    UCHAR gps_and_glo_mask;
};

struct BESTGNSSVEL
{
    LONG solution_status;
    LONG velocity_type;
    FLOAT latency;
    FLOAT diff_age;
    DOUBLE horizontal_speed;
    DOUBLE ground_track;
    DOUBLE vertical_speed;
    FLOAT BESTGNSSVEL_reserved1;
};

struct PPPPOS
{
    LONG solution_status;
    LONG position_type;
    DOUBLE latitude;
    DOUBLE longitude;
    DOUBLE orthometric_height;
    FLOAT undulation;
    LONG datum_id;
    FLOAT latitude_std_dev;
    FLOAT longitude_std_dev;
    FLOAT height_std_dev;
    CHAR base_id[4];
    FLOAT diff_age;
    FLOAT solution_age;
    UCHAR num_svs;
    UCHAR num_soln_svs;
    UCHAR num_soln_L1_svs;
    UCHAR num_soln_multi_svs;
    UCHAR extended_solution_status2;
    UCHAR ext_sol_stat;
    UCHAR gal_and_bds_mask;
    UCHAR gps_and_glo_mask;
};

struct PPPSATS_satellite_entries
{
    LONG system_type;
    SATELLITEID id;
    LONG status;
    ULONG status_mask;
};

struct PPPSATS
{
    ULONG satellite_entries_arraylength;
    PPPSATS_satellite_entries satellite_entries[325];
};

struct PPPDOP2_tdo_ps
{
    LONG system;
    FLOAT dop;
};

struct PPPDOP2
{
    FLOAT gdop;
    FLOAT pdop;
    FLOAT hdop;
    FLOAT vdop;
    ULONG tdo_ps_arraylength;
    PPPDOP2_tdo_ps tdo_ps[5];
};

struct HEADINGRATE
{
    LONG solution_status;
    LONG solution_type;
    FLOAT latency;
    FLOAT b_length;
    FLOAT heading;
    FLOAT pitch;
    FLOAT b_length_std_dev;
    FLOAT heading_std_dev;
    FLOAT pitch_std_dev;
    FLOAT float;
    CHAR rover_id[4];
    CHAR base_id[4];
    UCHAR extended_solution_status2;
    UCHAR char_as_int;
    UCHAR char_as_int_14;
    UCHAR char_as_int_15;
};

struct LBANDBEAMTABLE_items
{
    CHAR name[8];
    CHAR region_id[8];
    ULONG frequency_in_hz;
    ULONG baud_rate;
    FLOAT longitude;
    ULONG beam_access;
};

struct LBANDBEAMTABLE
{
    ULONG items_arraylength;
    LBANDBEAMTABLE_items items[32];
};

struct TERRASTARINFO
{
    CHAR pac[16];
    LONG operating_mode;
    ULONG subscription_details;
    ULONG contract_end_day_of_year;
    ULONG contract_end_year;
    ULONG timed_enable_period;
    LONG region_restriction;
    FLOAT local_area_center_point_latitude;
    FLOAT local_area_center_point_longitude;
    ULONG local_area_radius;
};

struct VERIPOSINFO
{
    ULONG serial_number;
    LONG operating_mode;
    ULONG subscription_details;
    CHAR service_code[4];
};

struct TERRASTARSTATUS
{
    LONG access_status;
    LONG decoder_sync_state;
    ULONG timed_enable_remaining_time;
    LONG local_area_status;
    LONG geogating_status;
};

struct VERIPOSSTATUS
{
    LONG access_status;
    LONG decoder_sync_state;
};

struct MARK3POS
{
    LONG solution_status;
    LONG position_type;
    DOUBLE latitude;
    DOUBLE longitude;
    DOUBLE orthometric_height;
    FLOAT undulation;
    LONG datum_id;
    FLOAT latitude_std_dev;
    FLOAT longitude_std_dev;
    FLOAT height_std_dev;
    CHAR base_id[4];
    FLOAT diff_age;
    FLOAT solution_age;
    UCHAR num_svs;
    UCHAR num_soln_svs;
    UCHAR num_soln_L1_svs;
    UCHAR num_soln_multi_svs;
    UCHAR char_as_int;
    UCHAR ext_sol_stat;
    UCHAR gal_and_bds_mask;
    UCHAR gps_and_glo_mask;
};

struct MARK4POS
{
    LONG solution_status;
    LONG position_type;
    DOUBLE latitude;
    DOUBLE longitude;
    DOUBLE orthometric_height;
    FLOAT undulation;
    LONG datum_id;
    FLOAT latitude_std_dev;
    FLOAT longitude_std_dev;
    FLOAT height_std_dev;
    CHAR base_id[4];
    FLOAT diff_age;
    FLOAT solution_age;
    UCHAR num_svs;
    UCHAR num_soln_svs;
    UCHAR num_soln_L1_svs;
    UCHAR num_soln_multi_svs;
    UCHAR char_as_int;
    UCHAR ext_sol_stat;
    UCHAR gal_and_bds_mask;
    UCHAR gps_and_glo_mask;
};

struct SAVEDSURVEYPOSITIONS_positions
{
    UCHAR id[5];
    DOUBLE latitude;
    DOUBLE longitude;
    DOUBLE height;
};

struct SAVEDSURVEYPOSITIONS
{
    ULONG positions_arraylength;
    SAVEDSURVEYPOSITIONS_positions positions[32];
};

struct PDPDOP2_tdo_ps
{
    LONG system;
    FLOAT dop;
};

struct PDPDOP2
{
    FLOAT gdop;
    FLOAT pdop;
    FLOAT hdop;
    FLOAT vdop;
    ULONG tdo_ps_arraylength;
    PDPDOP2_tdo_ps tdo_ps[5];
};

struct PPPDOP
{
    FLOAT gdop;
    FLOAT pdop;
    FLOAT hdop;
    FLOAT htdop;
    FLOAT tdop;
    FLOAT gps_elev_mask;
    ULONG sats_arraylength;
    ULONG sats[325];
};

struct PDPDOP
{
    FLOAT gdop;
    FLOAT pdop;
    FLOAT hdop;
    FLOAT htdop;
    FLOAT tdop;
    FLOAT gps_elev_mask;
    ULONG sats_arraylength;
    ULONG sats[325];
};

struct DUALANTENNAHEADING
{
    LONG solution_status;
    LONG position_type;
    FLOAT b_length;
    FLOAT heading;
    FLOAT pitch;
    FLOAT float;
    FLOAT heading_std_dev;
    FLOAT pitch_std_dev;
    CHAR base_id[4];
    UCHAR num_svs;
    UCHAR num_soln_svs;
    UCHAR num_soln_L1_svs;
    UCHAR num_soln_multi_svs;
    UCHAR extended_solution_status2;
    UCHAR ext_sol_stat;
    UCHAR gal_and_bds_mask;
    UCHAR gps_and_glo_mask;
};

struct GPHDTDUALANTENNA
{
    LONG solution_status;
    LONG position_type;
    BOOL output_hdt;
    FLOAT heading;
    UCHAR system_set;
};

struct RTKASSISTSTATUS
{
    LONG state;
    LONG status;
    FLOAT remaining_time;
    FLOAT corrections_age;
};

struct OCEANIXINFO
{
    CHAR pac[16];
    LONG operating_mode;
    ULONG subscription_details;
    ULONG contract_end_day_of_year;
    ULONG contract_end_year;
    ULONG timed_enable_period;
    LONG region_restriction;
};

struct OCEANIXSTATUS
{
    LONG access_status;
    LONG decoder_sync_state;
    LONG region_restriction_status;
};

struct BESTVELX
{
    LONG status;
    LONG vel_type;
    DOUBLE x;
    DOUBLE y;
    DOUBLE z;
    FLOAT x_std_dev;
    FLOAT y_std_dev;
    FLOAT z_std_dev;
    ULONG rsvd_field;
    USHORT latency_ms;
};

struct PPPSEEDAPPLICATIONSTATUS
{
    LONG status;
    DOUBLE latitude;
    DOUBLE longitude;
    DOUBLE height;
    FLOAT latitude_std_dev;
    FLOAT longitude_std_dev;
    FLOAT height_std_dev;
};

struct TECTONICSCOMPENSATION
{
    LONG status;
    CHAR name[32];
    FLOAT x;
    FLOAT y;
    FLOAT z;
};

struct PPPDATUMINFO
{
    CHAR name[32];
    ULONG epsg_code;
    DOUBLE epoch;
    LONG transformation_status;
};

struct PSRDATUMINFO
{
    CHAR name[32];
    ULONG epsg_code;
    DOUBLE epoch;
    LONG transformation_status;
};

struct BESTGNSSDATUMINFO
{
    CHAR name[32];
    ULONG epsg_code;
    DOUBLE epoch;
    LONG transformation_status;
};

struct PDPDATUMINFO
{
    CHAR name[32];
    ULONG epsg_code;
    DOUBLE epoch;
    LONG transformation_status;
};

struct RTKDATUMINFO
{
    CHAR name[32];
    ULONG epsg_code;
    DOUBLE epoch;
    LONG transformation_status;
};

struct BESTDATUMINFO
{
    CHAR name[32];
    ULONG epsg_code;
    DOUBLE epoch;
    LONG transformation_status;
};

struct WIFIAPSETTINGS
{
    UCHAR ssid[33];
    UCHAR passkey[65];
    LONG band;
    LONG security_type;
    LONG encryption;
    LONG region;
    INT channel;
    UCHAR bssid[18];
};

struct SATELSTATUS
{
    LONG satel_status;
    LONG satel_error;
    UCHAR failed_command[48];
};

struct WIFISTATUS_clients
{
    UCHAR mac_address[18];
};

struct WIFISTATUS
{
    LONG status;
    UCHAR ssid[33];
    INT rssi;
    ULONG channel;
    UCHAR bssid[18];
    LONG security;
    ULONG clients_arraylength;
    WIFISTATUS_clients clients[4];
};

struct WIFINETLIST_net_ap_info
{
    UCHAR ssid[33];
    INT rssi;
    ULONG channel;
    UCHAR bssid[18];
    LONG security;
};

struct WIFINETLIST
{
    ULONG net_ap_info_arraylength;
    WIFINETLIST_net_ap_info net_ap_info[20];
};

struct SATEL4INFO
{
    LONG satel_protocol;
    UINT tx_freq_hz;
    UINT rx_freq_hz;
    UINT channel_spacing_hz;
    UINT tx_power_m_w;
    BOOL fec_enabled;
};

struct SATEL9INFO
{
    LONG satel9_modem_mode;
    INT leica_channel;
    INT novariant_channel;
    UINT freq_key;
    UINT network_id;
    UINT min_packet_size;
    UINT max_packet_size;
    UINT retry_timeout;
    UINT subnet;
    BOOL repeaters;
    UINT master_packet_repeat;
    UINT tx_power_m_w;
    UINT hop_table_version;
    CHAR freq_zone[16];
};


#pragma pack()