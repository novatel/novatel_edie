#include <algorithm>
#include <cmath>
#include <cstring>

#ifndef NOVATEL_MESSAGE_DEFINITIONS_HPP
#define NOVATEL_MESSAGE_DEFINITIONS_HPP

#ifdef PASSTHROUGH
#undef PASSTHROUGH // Fix name collision in wingdi.h (included by spdlog)
#endif

namespace novatel::edie::oem {

#pragma pack(1)

struct VERSION_versions
{
    int32_t component_type{0};
    uint8_t model_name[16]{0};
    uint8_t psn[16]{0};
    uint8_t hardware_version[16]{0};
    uint8_t software_version[16]{0};
    uint8_t boot_version[16]{0};
    uint8_t compile_date[12]{0};
    uint8_t compile_time[12]{0};

    constexpr VERSION_versions() = default;
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

    bool operator==(const TRACKSTAT_chan_status& other) const
    {
        constexpr double dEpsilon = 1e-3;

        return prn == other.prn && freq == other.freq && channel_status == other.channel_status && psr_range_reject == other.psr_range_reject &&
               fabs(psr - other.psr) < dEpsilon && fabs(doppler - other.doppler) < dEpsilon && fabs(C_No - other.C_No) < dEpsilon &&
               fabs(lock_time - other.lock_time) < dEpsilon && fabs(psr_residual - other.psr_residual) < dEpsilon &&
               fabs(psr_filter_weighting - other.psr_filter_weighting) < dEpsilon;
    }
};

struct TRACKSTAT
{
    int32_t position_status;
    int32_t position_type;
    float tracking_elevation_cutoff;
    uint32_t chan_status_arraylength;
    TRACKSTAT_chan_status chan_status[325];

    bool operator==(const TRACKSTAT& other) const
    {
        constexpr double dEpsilon = 1e-3;

        return position_status == other.position_status && position_status == other.position_status &&
               chan_status_arraylength == other.chan_status_arraylength &&
               fabs(tracking_elevation_cutoff - other.tracking_elevation_cutoff) < dEpsilon &&
               std::equal(std::begin(chan_status), &chan_status[chan_status_arraylength], std::begin(other.chan_status));
    }
};

struct RAWGPSSUBFRAME
{
    int32_t frame_decoder_number;
    uint32_t satellite_id;
    uint32_t sub_frame_id;
    uint8_t raw_sub_frame_data[30];
    uint32_t signal_channel_number;

    bool operator==(const RAWGPSSUBFRAME& other) const { return memcmp(this, &other, sizeof(*this)) == 0; }
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

    bool operator==(const BESTPOS& other) const
    {
        constexpr double dEpsilon = 1e-4;
        constexpr double dCoordinateEpsilon = 1e-11;

        return solution_status == other.solution_status && position_type == other.position_type && datum_id == other.datum_id &&
               num_svs == other.num_svs && num_soln_svs == other.num_soln_svs && num_soln_L1_svs == other.num_soln_L1_svs &&
               num_soln_multi_svs == other.num_soln_multi_svs && extended_solution_status2 == other.extended_solution_status2 &&
               ext_sol_stat == other.ext_sol_stat && gal_and_bds_mask == other.gal_and_bds_mask && gps_and_glo_mask == other.gps_and_glo_mask &&
               fabs(latitude - other.latitude) < dCoordinateEpsilon && fabs(longitude - other.longitude) < dCoordinateEpsilon &&
               fabs(orthometric_height - other.orthometric_height) < dEpsilon && fabs(undulation - other.undulation) < dEpsilon &&
               fabs(latitude_std_dev - other.latitude_std_dev) < dEpsilon && fabs(longitude_std_dev - other.longitude_std_dev) < dEpsilon &&
               fabs(height_std_dev - other.height_std_dev) < dEpsilon && fabs(diff_age - other.diff_age) < dEpsilon &&
               fabs(solution_age - other.solution_age) < dEpsilon && memcmp(base_id, other.base_id, sizeof(base_id)) == 0;
    }
};

struct LOGLIST_log_list
{
    int32_t log_port_address;
    uint32_t message_id;
    int32_t trigger;
    double on_time;
    double offset;
    int32_t hold;

    bool operator==(const LOGLIST_log_list& other) const
    {
        constexpr double dEpsilon = 1e-4;

        return log_port_address == other.log_port_address && message_id == other.message_id && trigger == other.trigger && hold == other.hold &&
               fabs(on_time - other.on_time) < dEpsilon && fabs(offset - other.offset) < dEpsilon;
    }
};

struct LOGLIST
{
    uint32_t log_list_arraylength;
    LOGLIST_log_list log_list[80];

    bool operator==(const LOGLIST& other) const
    {
        return log_list_arraylength == other.log_list_arraylength &&
               std::equal(std::begin(log_list), &log_list[log_list_arraylength], std::begin(other.log_list));
    }
};

struct BESTSATS_satellite_entries
{
    int32_t system_type;
    SATELLITEID id;
    int32_t status;
    uint32_t status_mask;

    bool operator==(const BESTSATS_satellite_entries& other) const
    {
        return system_type == other.system_type && id == other.id && status == other.status && status_mask == other.status_mask;
    }
};

struct BESTSATS
{
    uint32_t satellite_entries_arraylength;
    BESTSATS_satellite_entries satellite_entries[325];

    bool operator==(const BESTSATS& other) const
    {
        return satellite_entries_arraylength == other.satellite_entries_arraylength &&
               std::equal(std::begin(satellite_entries), &satellite_entries[satellite_entries_arraylength], std::begin(other.satellite_entries));
    }
};

#pragma pack()
} // namespace novatel::edie::oem

#endif // NOVATEL_MESSAGE_DEFINITIONS_HPP
