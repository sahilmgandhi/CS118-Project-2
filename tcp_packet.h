class TCP_Packet {

  struct TCP_Header {
    // add fields
    // set the Flags
    // set the cwnd for extra credit portion

  } m_header;

  // Store the data in a vector
  // Mark packet as sent and acked as necessary
  // Packets by default arent acked or sent

  bool sent = false;
  bool acked = false;
};