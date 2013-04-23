/**
 * @cond internal
 * @file
 */
#include <avarix/intlvl.h>
#include "ax12.h"


/// Compute packet checksum
uint8_t ax12_checksum(const ax12_pkt_t *pkt)
{
  uint8_t checksum = 0;
  checksum += pkt->id;
  checksum += pkt->instruction;
  checksum += pkt->nparams + 2;
  for(uint8_t i=0; i<pkt->nparams; i++) {
    checksum += pkt->params[i];
  }
  return ~checksum;
}


uint8_t ax12_send(ax12_t *s, const ax12_pkt_t *pkt)
{
  if(pkt->nparams > AX12_MAX_PARAMS) {
    return AX12_ERROR_INVALID_PACKET;
  }

  // switch line to write
  s->set_state(AX12_STATE_WRITE);

  // send header
  uint8_t header[] = {
    0xFF, 0xFF, pkt->id, pkt->nparams+2,
    pkt->instruction,
  };
  for(uint8_t i=0; i<sizeof(header); i++) {
    if(s->send(header[i])) {
      goto fail;
    }
  }

  // send parameters
  for(uint8_t i=0; i<pkt->nparams; i++) {
    if(s->send(pkt->params[i])) {
      goto fail;
    }
  }

  // Lock before sending last byte and before switching state to avoid
  // receiving the reply before the switch; this is required with asynchronous
  // receiving.
  INTLVL_DISABLE_ALL_BLOCK() {
    // checksum
    if(s->send(ax12_checksum(pkt))) {
      goto fail;
    }
    s->set_state(AX12_STATE_READ);
  }

  return 0;

 fail:
  s->set_state(AX12_STATE_READ);
  return AX12_ERROR_SEND_FAILED; // only possible error
}


uint8_t ax12_recv(ax12_t *s, ax12_pkt_t *pkt)
{
  int c;

  s->set_state(AX12_STATE_READ);

  // start bytes
  c = s->recv();
  if(c == -1) {
    return AX12_ERROR_NO_REPLY;
  } else if(c != 0xFF) {
    return AX12_ERROR_INVALID_PACKET;
  }
  c = s->recv();
  if(c == -1) {
    return AX12_ERROR_REPLY_TIMEOUT;
  } else if(c != 0xFF) {
    return AX12_ERROR_INVALID_PACKET;
  }

  // ID
  c = s->recv();
  if(c == -1) {
    return AX12_ERROR_REPLY_TIMEOUT;
  }
  pkt->id = c;

  // length
  c = s->recv();
  if(c == -1) {
    return AX12_ERROR_REPLY_TIMEOUT;
  }
  pkt->nparams = c - 2;
  if(pkt->nparams > AX12_MAX_PARAMS) {
    return AX12_ERROR_INVALID_PACKET;
  }

  // error
  c = s->recv();
  if(c == -1) {
    return AX12_ERROR_REPLY_TIMEOUT;
  }
  pkt->error = c;
  if(pkt->error) {
    return pkt->error;
  }

  // parameters
  for(uint8_t i=0; i<pkt->nparams; i++) {
    c = s->recv();
    if(c == -1) {
      return AX12_ERROR_REPLY_TIMEOUT;
    }
    pkt->params[i] = c;
  }

  // checksum
  c = s->recv();
  if(c != ax12_checksum(pkt)) {
    return AX12_ERROR_BAD_CHECKSUM;
  }

  return 0;
}


uint8_t ax12_write_byte(ax12_t *s, uint8_t id, ax12_addr_t addr, uint8_t data)
{
  ax12_pkt_t pkt = {
    .id = id,
    .instruction = AX12_INSTR_WRITE,
    .nparams = 2,
    .params = { addr, data },
  };

  uint8_t ret;
  if((ret = ax12_send(s, &pkt))) {
    return ret;
  }
  if(pkt.id == AX12_BROADCAST_ID) {
    return 0;  // no reply on broadcast
  }

  return ax12_recv(s, &pkt);
}

uint8_t ax12_write_word(ax12_t *s, uint8_t id, ax12_addr_t addr, uint16_t data)
{
  ax12_pkt_t pkt = {
    .id = id,
    .instruction = AX12_INSTR_WRITE,
    .nparams = 3,
    .params = { addr, data & 0xFF, data >> 8 },
  };

  uint8_t ret;
  if((ret = ax12_send(s, &pkt))) {
    return ret;
  }
  if(pkt.id == AX12_BROADCAST_ID) {
    return 0;  // no reply on broadcast
  }

  return ax12_recv(s, &pkt);
}

uint8_t ax12_write_mem(ax12_t *s, uint8_t id, ax12_addr_t addr, uint8_t n, const uint8_t *data)
{
  if(n >= AX12_MAX_PARAMS) {
    return AX12_ERROR_INVALID_PACKET;
  }
  ax12_pkt_t pkt = {
    .id = id,
    .instruction = AX12_INSTR_WRITE,
    .nparams = n+1,
    .params = { addr },
  };
  for(uint8_t i=0; i<n; i++) {
    pkt.params[i+1] = data[i];
  }

  uint8_t ret;
  if((ret = ax12_send(s, &pkt))) {
    return ret;
  }
  if(pkt.id == AX12_BROADCAST_ID) {
    return 0;  // no reply on broadcast
  }

  return ax12_recv(s, &pkt);
}

uint8_t ax12_read_byte(ax12_t *s, uint8_t id, ax12_addr_t addr, uint8_t *data)
{
  ax12_pkt_t pkt = {
    .id = id,
    .instruction = AX12_INSTR_READ,
    .nparams = 2,
    .params = { addr, 1 },
  };

  uint8_t ret;
  if((ret = ax12_send(s, &pkt))) {
    return ret;
  }
  if((ret = ax12_recv(s, &pkt))) {
    return ret;
  }

  *data = pkt.params[0];
  return 0;
}

uint8_t ax12_read_word(ax12_t *s, uint8_t id, ax12_addr_t addr, uint16_t *data)
{
  ax12_pkt_t pkt = {
    .id = id,
    .instruction = AX12_INSTR_READ,
    .nparams = 2,
    .params = { addr, 2 },
  };

  uint8_t ret;
  if((ret = ax12_send(s, &pkt))) {
    return ret;
  }
  if((ret = ax12_recv(s, &pkt))) {
    return ret;
  }

  *data = pkt.params[0] | (pkt.params[1] << 8);
  return 0;
}

uint8_t ax12_read_mem(ax12_t *s, uint8_t id, ax12_addr_t addr, uint8_t n, uint8_t *data)
{
  if(n >= AX12_MAX_PARAMS) {
    return AX12_ERROR_INVALID_PACKET;
  }
  ax12_pkt_t pkt = {
    .id = id,
    .instruction = AX12_INSTR_READ,
    .nparams = 2,
    .params = { addr, n },
  };

  uint8_t ret;
  if((ret = ax12_send(s, &pkt))) {
    return ret;
  }
  if((ret = ax12_recv(s, &pkt))) {
    return ret;
  }

  for(uint8_t i=0; i<n; i++) {
    data[i] = pkt.params[0];
  }
  return 0;
}


uint8_t ax12_ping(ax12_t *s, uint8_t id)
{
  ax12_pkt_t pkt = {
    .id = id,
    .instruction = AX12_INSTR_PING,
    .nparams = 0,
    .params = {},
  };

  uint8_t ret;
  if((ret = ax12_send(s, &pkt))) {
    return ret;
  }
  if(pkt.id == AX12_BROADCAST_ID) {
    return 0;  // no reply on broadcast
  }

  return ax12_recv(s, &pkt);
}

uint8_t ax12_reset(ax12_t *s, uint8_t id)
{
  ax12_pkt_t pkt = {
    .id = id,
    .instruction = AX12_INSTR_RESET,
    .nparams = 0,
    .params = {},
  };

  uint8_t ret;
  if((ret = ax12_send(s, &pkt))) {
    return ret;
  }
  if(pkt.id == AX12_BROADCAST_ID) {
    return 0;  // no reply on broadcast
  }

  return ax12_recv(s, &pkt);
}


//@endcond
