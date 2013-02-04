#ifndef PROTOCOL_HPP__
#define PROTOCOL_HPP__

#include "simple_command.pb.h"
#include <pb_decode.h>
#include <pb_encode.h>

#include <Arduino.h>

typedef bool (*writer_func)(uint8_t* data, uint8_t size);


const pb_field_t* decode_unionmessage_type(pb_istream_t *stream,  const pb_field_t uniontype[]);
bool decode_unionmessage_contents(pb_istream_t *stream, const pb_field_t fields[], void *dest_struct);
bool send_package(const pb_field_t messagetype[], const void *message, writer_func func);


void debug_print(char const* c, writer_func func);




extern pb_istream_t istream;

extern DebugPrint debug_print_message;

#endif