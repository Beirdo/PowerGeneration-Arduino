#ifndef SERIALCLI_H__
#define SERIALCLI_H__

#include <Stream.h>
#include "linkedlist.h"

class SerialCLI;

class CLICommand {
    public:
        CLICommand(const char *command, uint8_t nargs);
        void attach(SerialCLI *cli);
        uint8_t compare(const char *command);
        virtual uint8_t run(uint8_t nargs, uint8_t **args) = 0;
        uint8_t pre_run(uint8_t nargs);
        uint8_t nargs(void) { return m_nargs; };
        const char *command(void) { return m_command; };
        Stream *serial(void);

    protected:
        SerialCLI *m_cli;
        const char *m_command;
        uint8_t m_len;
        uint8_t m_nargs;
};

#define SERIAL_BUFFER_SIZE 16

class SerialCLI {
    public:
#ifdef __AVR__
        SerialCLI(HardwareSerial &serial, uint32_t baud = 115200);
#endif
#ifdef __arm__
        SerialCLI(Serial_ &serial, uint32_t baud = 115200);
#endif
        void initialize(void);
        void registerCommand(CLICommand *command);
        void listCommands(void);
        void handleInput(void);
        Stream *serial(void) { return (Stream *)&m_serial; };

    protected:
        bool connect(void);
        void parseBuffer(void);
        void registerCommonCommands(void);
        void prompt(void) { m_serial.print("> "); };

        LinkedList m_commands;
        uint8_t m_index;
        char m_buffer[SERIAL_BUFFER_SIZE];
#ifdef __AVR__
        HardwareSerial &m_serial;
#endif
#ifdef __arm__
        Serial_ &m_serial;
#endif
        uint32_t m_baud;
        bool m_connected;
};

#endif
// vim:ts=4:sw=4:ai:et:si:sts=4
