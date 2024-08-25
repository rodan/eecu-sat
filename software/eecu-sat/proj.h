#ifndef SIGROK_GLUE_H
#define SIGROK_GLUE_H

#define            CHUNK_SIZE  (8 * 1024 * 1024)
#define           LINE_MAX_SZ  64

#define     ACTION_DO_CONVERT  0x01
#define ACTION_DO_CALIBRATION  0x02
#define  ACTION_DO_CALIB_INIT  0x04

enum sr_packettype {
	/** Payload is sr_datafeed_header. */
	SR_DF_HEADER = 10000,
	/** End of stream (no further data). */
	SR_DF_END,
	/** Payload is struct sr_datafeed_meta */
	SR_DF_META,
	/** The trigger matched at this point in the data feed. No payload. */
	SR_DF_TRIGGER,
	/** Payload is struct sr_datafeed_logic. */
	SR_DF_LOGIC,
	/** Beginning of frame. No payload. */
	SR_DF_FRAME_BEGIN,
	/** End of frame. No payload. */
	SR_DF_FRAME_END,
	/** Payload is struct sr_datafeed_analog. */
	SR_DF_ANALOG,

	/* Update datafeed_dump() (session.c) upon changes! */
};

/** Packet in a sigrok data feed. */
struct sr_datafeed_packet {
	enum sr_packettype type;
    ssize_t payload_size;
	void *payload;
};

#define UNUSED(x) (void)(x)

#endif
