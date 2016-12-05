#ifndef SERIALCLI_H__
#define SERIALCLI_H__

#include "linkedlist.h"

class CLICommand {
    public:
        CLICommand(uint8_t *command, uint8_t nargs);
        uint8_t compare(uint8_t *command);
        virtual uint8_t run(uint8_t nargs, uint8_t **args) = 0;
        uint8_t pre_run(uint8_t nargs);
        uint8_t nargs(void) { return m_nargs; };
        uint8_t *command(void) { return m_command; };

    protected:

        uint8_t *m_command;
        uint8_t m_len;
        uint8_t m_nargs;
};

#define SERIAL_BUFFER_SIZE 128

class SerialCLI {
    public:
        SerialCLI(void);
        void initialize(void);
        void registerCommand(CLICommand *command);
        void listCommands(void);
        void handleInput(void);

    protected:
        void parseBuffer(void);
        void registerCommonCommands(void);
        void prompt(void) { Serial.print("> "); };
        LinkedList m_commands;
        uint8_t m_index;
        uint8_t m_buffer[SERIAL_BUFFER_SIZE];
};

extern SerialCLI cli;

#endif
// vim:ts=4:sw=4:ai:et:si:sts=4
