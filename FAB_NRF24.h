#ifndef FAB_NRF24
#define FAB_NRF24

#ifndef RF24
#error FAB_NRF24 requires the RF24 library to compile
#endif

////////////////////////
/// COMMAND SET LIST ///
////////////////////////

/// FAB_NRF_CMD_LIST defines the library's supported command set.
/// A command will have a name that you use, defined in FabNrfCommands,
/// wich represents a numeric key index (ID1.ID0). ID1 defines a group
/// of commands of similar type (0..255). ID0 defines an actual action
/// command of that group.

/// Predefined command groups:
#define FAB_NRF_GRP_MAIN 0	// Base command set used by most
#define FAB_NRF_GRP_DMX  1	// DMX compatible command set
#define FAB_NRF_GRP_EXT  2	// Extended command set 
#define FAB_NRF_GRP_USR  9	// Reserved for user commands

/// Supported commands:

/// Definition format:
/// #define FAB_NRF_CMD(id1, id0, nam, dsc);
/// * id1: Group
/// * id0: Command index (0..64K`)
/// * siz: Number of bytes of packet excluding id (255 means variable)
///        Variable size packets will encode the size in the packet as needed
/// * nam: Command name (in the code)
/// * dsc:  Description
#define FAB_NRF_CMD_LIST                                                       \
	FAB_NRF_CMD(FAB_NRF_GRP_MAIN, 0,   0, FAB_NRF_CMD_PING, ping)          \
	FAB_NRF_CMD(FAB_NRF_GRP_MAIN, 1,   0, FAB_NRF_CMD_ACK, acknowledge)    \
	FAB_NRF_CMD(FAB_NRF_GRP_MAIN, 2,   1, FAB_NRF_CMD_ON, turn target on)  \
	FAB_NRF_CMD(FAB_NRF_GRP_MAIN, 3,   1, FAB_NRF_CMD_OFF, turn target off)\
	FAB_NRF_CMD(FAB_NRF_GRP_MAIN, 4,   0, FAB_NRF_CMD_MAIN_LAST, last idx) \
\
	FAB_NRF_CMD(FAB_NRF_GRP_DMX, 0,   0, FAB_NRF_CMD_DMX_LAST, last idx) \
\
	FAB_NRF_CMD(FAB_NRF_GRP_EXT, 0,   0, FAB_NRF_CMD_EXT_LAST, last idx) \
\
	FAB_NRF_CMD(FAB_NRF_GRP_USR, 0,   0, FAB_NRF_CMD_USR_LAST, last idx) \


// FabNrfCommands: List of commands supported by the library
// Usage:
// void foo(const FabNrfCommands cmd) {if (cmd == FAB_NRF_CMD_PING){...}}
#define FAB_NRF_CMD(id1, id0, nam, dsc) name = ((int)id1)<<8 + id0,
enum FabNrfCommands : uint8_t
{
	FAB_NRF_CMD_LIST
	FAB_NRF_CMD_ERROR // Past last valid command
};
#undef FAB_NRF_CMD

///////////////////////////
/// COMMAND DESCRIPTION ///
///////////////////////////
#if 0
FAB_NRF_CMD_PING: Request receiver to send  FAB_NRF_CMD_ACK
	todo: PING is a group, and can be used to unstuck a receiver if
	there's a frame error?
FAB_NRF_CMD_ACK:  Acknowledge reception.
	todo: Returns last command received.
FAB_NRF_CMD_ON:   Turn on receiver(index)
FAB_NRF_CMD_OFF:  Turn off receiver(index)
#endif

////////////////////
/// FRAME FORMAT ///
////////////////////

// We will send one byte stream per command
// * The address and channel is handled by NRF24, so it is not encoded.
// * 2 byte command id1.id0


#define FAB_NRF_LAST_ADDRESS 4

class fabNrf
{
private:
	typedef union {
		struct {
			uint8_t grp;
			FabNrfCommands cmd;
			uint16_t num;
		} d3;
		struct {
			uint16_t command;
			uint16_t number;
		} d2;
		uint32_t blk;
	} t_CmdBlock;

	const RF24 & c_radio;

	// Registered callbacks for all command set
	uint16_t (*v_main_handlers[FAB_NRF_CMD_MAIN_LAST])(const char address[6], const uint16_t number );
	uint16_t (*v_dmx_handlers[ FAB_NRF_CMD_DMX_LAST ])(const char address[6], const uint16_t number );
	uint16_t (*v_ext_handlers[ FAB_NRF_CMD_EXT_LAST ])(const char address[6], const uint16_t number );
	uint16_t (*v_usr_handlers[ FAB_NRF_CMD_USR_LAST ])(const char address[6], const uint16_t number );

	// Supported addresses (max 4 or 6 bytes)
	char v_addresses[6][FAB_NRF_LAST_ADDRESS];
	uint8_t v_lastAddress;

	uint16_t v_lastCommand;

public:
	uint8_t v_ack;
	uint8_t v_toggle;
	uint8_t v_on;
	uint8_t v_off;

	void fabNrf(RF24 & radio, unsigned channel)
		c_radio(radio),
		v_main_handlers({0}),
		v_dmx_handlers({0}),
		v_ext_handlers({0}),
		v_usr_handlers({0}),
		v_addresses({0}),
		v_lastAddress(0),
		v_lastCommand(0xFFFF),
		v_ack(0xFF),
		v_toggle(0xFF),
		v_on(0xFF),
		v_off(0xFF),
	{
		// Configure radio
		c_radio.begin();
		c_radio.setPALevel(RF24_PA_HIGH);
		c_radio.setChannel(channel);
		radio.stopListening();
	};

	void ~fabNrf(void) {};

	///////////////////////////////////////////////////////////////////////////////

	/// @brief Prepare to send data to given address
	void initSend(void)
	{
		radio.openWritingPipe(address);
	}

	// @brief send a command
	// This is very generic, there's 3 modes:
	// If there's a buffer, send the buffer size as the number, unless it is set to zero,
	// in which case the receiver knows the size of the buffer and number is used to hold data.
	// cmd, value=val
	// cmd, value=*buf, val=sizeof(buf)
	// cmd, value1=val, value2=*buf
	void send(FabNrfCommands cmd, const char address[6], const uint16_t val, const uint16_t bufSz, void * buf)
	{
		radio.openWritingPipe(address);

		t_CmdBlock cmdBlock;
		cmdBlock.d2.command = cmd;
		cmdBlock.d2.number = (buf == null || bufSz == 0) ? val : bufSz;
		c_radio.write(&cmdBlock, sizeof(uint32_t));

		if (buf != null)
			c_radio.write(buf, bufSz);

		radio.closeWritingPipe(address);
	}


	///////////////////////////////////////////////////////////////////////////////

	/// @brief Configure to receive a command at an address
	void initReceive(const char * address)
	{
		// Initialize default callbacks
		if (!v_main_handlers[FAB_NRF_CMD_PING])
			v_main_handlers[FAB_NRF_CMD_PING] = pingHandler;
		if (!v_main_handlers[FAB_NRF_CMD_TOGGLE])
			v_main_handlers[FAB_NRF_CMD_TOGGLE] = toggleHandler;
		if (!v_main_handlers[FAB_NRF_CMD_ON])
			v_main_handlers[FAB_NRF_CMD_ON] = onHandler;
		if (!v_main_handlers[FAB_NRF_CMD_OFF])
			v_main_handlers[FAB_NRF_CMD_OFF] = offHandler;

		// Too many addresses to track
		if (v_lastAddress >= FAB_NRF_LAST_ADDRESS)
			return false;
		}
		c_radio.openReadingPipe(0, address);
		c_radio.startListening();

	}

	/// @brief Configure the command handlers for supported operations
	void initReceiveHandler(
		FabNrfCommands command,
		void (*commandHandler)(const char address[6], uint16_t number))
	{
		uint8_t grp = command >> 8;
		uint8_t cmd = command & 255;
		switch (grp) {
			case FAB_NRF_GRP_MAIN:
				v_main_handlers[cmd] = commandHandler;
				break;
			case FAB_NRF_GRP_DMX:
				v_dmx_handlers[cmd] = commandHandler;
				break;
			case FAB_NRF_GRP_EXT:
				v_ext_handlers[cmd] = commandHandler;
				break;
			case FAB_NRF_GRP_USR:
				v_usr_handlers[cmd] = commandHandler;
				break;
		}
	}

	/// @brief Call this method to process messages received from
	/// the sender
	inline void receive(char * buffer, unsigned char size)
	{
		if (c_radio.available())
			_receive(buffer, size);
	}

	/// @brief Reads a block of data
	void read(uint16_t bufSize, uint8_t * buffer)
	{
		c_radio.read(buffer, bufSize);
	}

	void _receive(char * buffer, unsigned char size)
	{
		// Read the message
		t_CmdBlock cmdBlock;
		cmdBlock.d2.command = cmd;
		cmdBlock.d2.number = num;
		read(&cmdBlock.blk,sizeof(cmdBlock.blk));

		void (*hdl)(const char address[6], uint16_t number);
		switch (cmdBlock.d3.grp) {
			case FAB_NRF_GRP_MAIN:
				hdl = v_main_handlers[cmdBlock.d3.cmd];
				break;
			case FAB_NRF_GRP_DMX:
				hdl = v_dmx_handlers[cmdBlock.d3.cmd];
				break;
			case FAB_NRF_GRP_EXT:
				hdl = v_ext_handlers[cmdBlock.d3.cmd];
				break;
			case FAB_NRF_GRP_USR:
				hdl = v_usr_handlers[cmdBlock.d3.cmd];
				break;
			default:
				// Unsupported group
				return;
		}

		if (hdl != null)
		{
			// @todo get the address where the message is coming from.
			const char address[6] = "111111";
			// Call the handler for cmd, pass num.
			hdl(address, cmdBlock.d3.num);
		}

		v_lastCommand = cmdBlock.d3.cmd;
	}

	/// @brief If we receive a ping, send an ACK with the last command received
	void pingHandler(const char address[6], uint16_t number)
	{
		send(FAB_NRF_CMD_ACK, address, v_lastCommand, 0, null);
	}

	/// @brief Default toggle/on/off command handler. Sets internal toggle value.
	/// After reading it the user must clear the value to 0xFF to detect the next transition
	void ackHandler(const char address[6], uint16_t number)
	{
		v_ack = number & 0xFF;
	}
	void toggleHandler(const char address[6], uint16_t number)
	{
		v_toggle = number & 0xFF;
	}
	void onHandler(const char address[6], uint16_t number)
	{
		v_on = number & 0xFF;
	}
	void offHandler(const char address[6], uint16_t number)
	{
		v_off = number & 0xFF;
	}
}

#endif // FAB_NRF24
