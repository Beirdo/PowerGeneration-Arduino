#ifndef SERIALCLI_H__
#define SERIALCLI_H__

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
        HardwareSerial *serial(void) { return m_cli->serial(); };

    protected:
        SerialCLI *m_cli;
        const char *m_command;
        uint8_t m_len;
        uint8_t m_nargs;
};

#define SERIAL_BUFFER_SIZE 16

class SerialCLI {
    public:
        SerialCLI(HardwareSerial *serial = &Serial, uint32_t baud = 115200);
        void initialize(void);
        void registerCommand(CLICommand *command);
        void listCommands(void);
        void handleInput(void);
        HardwareSerial *serial(void) { return m_serial; };

    protected:
        void parseBuffer(void);
        void registerCommonCommands(void);
        void prompt(void) { Serial.print("> "); };
        LinkedList m_commands;
        uint8_t m_index;
        char m_buffer[SERIAL_BUFFER_SIZE];
        HardwareSerial *m_serial;
};

#endif
// vim:ts=4:sw=4:ai:et:si:sts=4
