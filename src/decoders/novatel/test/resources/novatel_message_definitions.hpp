#ifndef NOVATEL_MESSAGE_DEFINITIONS_HPP
#define NOVATEL_MESSAGE_DEFINITIONS_HPP

#ifdef PASSTHROUGH
   #undef PASSTHROUGH // Fix name collision in wingdi.h (included by spdlog)
#endif

namespace novatel::edie::oem {

#pragma pack(1)

struct VERSION_versions
{
	int32_t component_type;
	uint8_t model_name[16];
	uint8_t psn[16];
	uint8_t hardware_version[16];
	uint8_t software_version[16];
	uint8_t boot_version[16];
	uint8_t compile_date[12];
	uint8_t compile_time[12];
};

struct VERSION
{
	uint32_t versions_arraylength;
	VERSION_versions versions[20];
};

struct PORTSTATS_port_statistics
{
	int32_t port;
	uint32_t rx_chars;
	uint32_t tx_chars;
	uint32_t good_rx_chars;
	uint32_t dropped_chars;
	uint32_t interrupts;
	uint32_t breaks;
	uint32_t parity_errors;
	uint32_t framing_errors;
	uint32_t over_runs;
};

struct PORTSTATS
{
	uint32_t port_statistics_arraylength;
	PORTSTATS_port_statistics port_statistics[33];
};

struct VALIDMODELS_models
{
	uint8_t model[16];
	uint32_t expiry_year;
	uint32_t expiry_month;
	uint32_t expiry_day;
};

struct VALIDMODELS
{
	uint32_t models_arraylength;
	VALIDMODELS_models models[24];
};

struct PSRDOP
{
	float gdop;
	float pdop;
	float hdop;
	float htdop;
	float tdop;
	float gps_elev_mask;
	uint32_t sats_arraylength;
	uint32_t sats[325];
};

struct GLOEPHEMERIS
{
	uint16_t sloto;
	uint16_t freqo;
	uint8_t sat_type;
	uint8_t false_iod;
	uint16_t ephem_week;
	uint32_t ephem_time;
	uint32_t time_offset;
	uint16_t nt;
	uint8_t GLOEPHEMERIS_reserved;
	uint8_t GLOEPHEMERIS_reserved_9;
	uint32_t issue;
	uint32_t broadcast_health;
	double pos_x;
	double pos_y;
	double pos_z;
	double vel_x;
	double vel_y;
	double vel_z;
	double ls_acc_x;
	double ls_acc_y;
	double ls_acc_z;
	double tau;
	double delta_tau;
	double gamma;
	uint32_t tk;
	uint32_t p;
	uint32_t ft;
	uint32_t age;
	uint32_t flags;
};

struct TRACKSTAT_chan_status
{
	uint16_t prn;
	uint16_t freq;
	uint32_t channel_status;
	double psr;
	float doppler;
	float C_No;
	float lock_time;
	float psr_residual;
	int32_t psr_range_reject;
	float psr_filter_weighting;
};

struct TRACKSTAT
{
	int32_t position_status;
	int32_t position_type;
	float tracking_elevation_cutoff;
	uint32_t chan_status_arraylength;
	TRACKSTAT_chan_status chan_status[325];
};

struct RAWGPSSUBFRAME
{
	int32_t frame_decoder_number;
	uint32_t satellite_id;
	uint32_t sub_frame_id;
   uint8_t raw_sub_frame_data[30];
	uint32_t signal_channel_number;
};

struct BESTPOS
{
	int32_t solution_status;
	int32_t position_type;
	double latitude;
	double longitude;
	double orthometric_height;
	float undulation;
	int32_t datum_id;
	float latitude_std_dev;
	float longitude_std_dev;
	float height_std_dev;
	uint8_t base_id[4];
	float diff_age;
	float solution_age;
	uint8_t num_svs;
	uint8_t num_soln_svs;
	uint8_t num_soln_L1_svs;
	uint8_t num_soln_multi_svs;
	uint8_t extended_solution_status2;
	uint8_t ext_sol_stat;
	uint8_t gal_and_bds_mask;
	uint8_t gps_and_glo_mask;
};

struct LOGLIST_log_list
{
	int32_t log_port_address;
	uint32_t message_id;
	int32_t trigger;
	double on_time;
	double offset;
	int32_t hold;
};

struct LOGLIST
{
	uint32_t log_list_arraylength;
	LOGLIST_log_list log_list[80];
};

struct BESTSATS_satellite_entries
{
	int32_t system_type;
	SATELLITEID id;
	int32_t status;
	uint32_t status_mask;
};

struct BESTSATS
{
	uint32_t satellite_entries_arraylength;
	BESTSATS_satellite_entries satellite_entries[325];
};

#pragma pack()
}

#endif //NOVATEL_MESSAGE_DEFINITIONS_HPP
