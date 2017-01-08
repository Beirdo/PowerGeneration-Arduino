#include <Arduino.h>
#include <Stream.h>
#include <string.h>
#include "serialcli.h"

CLICommand::CLICommand(const char *command, uint8_t nargs)
{
    m_command = command;
    m_len = strlen(command);
    m_nargs = nargs;
    m_cli = NULL;
}

Stream *CLICommand::serial(void)
{
    return m_cli->serial();
}

void CLICommand::attach(SerialCLI *cli)
{
    m_cli = cli;
}

uint8_t CLICommand::compare(const char *command)
{
    uint8_t len = strlen(command);
    if (len > m_len) {
        len = m_len;
    }
    return (strncmp(m_command, command, len) == 0);
}

uint8_t CLICommand::pre_run(uint8_t nargs)
{
    return (nargs == m_nargs);
}

#ifdef __AVR__
SerialCLI::SerialCLI(HardwareSerial &serial, uint32_t baud) :
#endif
#ifdef __arm__
SerialCLI::SerialCLI(Serial_ &serial, uint32_t baud) :
#endif
        m_serial(serial)
{
    m_commands = LinkedList();
    m_index = 0;
    m_baud = baud;
    m_connected = false;
    registerCommonCommands();
}

void SerialCLI::initialize(void)
{
    m_connected = false;
}

bool SerialCLI::connect(void)
{
    if (!m_serial) {
        m_connected = false;
        return false;
    }

    m_connected = true;
    m_serial.begin(m_baud);

    m_serial.println("CLI Ready");
    prompt();
}

void SerialCLI::registerCommand(CLICommand *command)
{
    command->attach(this);
    m_commands.add((void *)command);
}

void SerialCLI::parseBuffer(void)
{
    char *ch;
    char *command;
    uint8_t nargs = 0;
    char *args[8];   // support a max of 8 args
    CLICommand *cmd;

    // Strip leading whitespace
    for (ch = m_buffer; *ch == ' ' || *ch == '\t'; ch++);

    // Blank line
    if (!*ch) {
        prompt();
        return;
    }

    command = ch;

    // Find end of command (whitespace)
    for ( ; *ch && *ch != ' ' && *ch != '\t'; ch++);

    // zero-terminate the command
    if (*ch) {
        *ch = '\0';
        ch++;
    }

    // Gather arguments
    while (*ch) {
        // Strip leading whitespace
        for ( ; *ch == ' ' || *ch == '\t'; ch++);

        // Nothing left
        if (!*ch) {
            continue;
        }

        // Set the beginning of an argument
        args[nargs++] = ch;

        // Find end of argument (whitespace)
        for ( ; *ch && *ch != ' ' && *ch != '\t'; ch++);

        // zero-terminate the argument
        if (*ch) {
            *ch = '\0';
            ch++;
        }
    }

    // Find the matching command
    for (cmd = (CLICommand *)m_commands.head(); cmd;
         cmd = (CLICommand *)m_commands.next()) {
        if (cmd->compare(command)) {
            break;
        }
    }

    // Do it if we have a match
    if (cmd) {
        if (!cmd->pre_run(nargs)) {
            m_serial.print("ERROR: Command ");
            m_serial.print((char *)cmd->command());
            m_serial.print(" requires ");
            m_serial.print(cmd->nargs());
            m_serial.println(" arguments");
        } else if (!cmd->run(nargs, (uint8_t **)args)) {
            m_serial.println("ERROR: Command failed");
        } else {
            m_serial.println("SUCCESS");
        }
    } else {
        m_serial.println("ERROR: Unknown command");
    }
    prompt();
}

class HelpCLICommand : public CLICommand {
    public:
        HelpCLICommand(void) : CLICommand("help", 0) {};
        virtual uint8_t run(uint8_t nargs, uint8_t **args) 
        {
            m_cli->listCommands();
            return(1);
        }
};

class ResetCLICommand : public CLICommand {
    public:
        ResetCLICommand(void) : CLICommand("reset", 0) {};
        virtual uint8_t run(uint8_t nargs, uint8_t **args) 
        { 
            serial()->println("Resetting...");
            delay(1000);
#ifdef __AVR__
            // Jump to the reset vector
            asm volatile ("jmp 0");
#endif

#ifdef __arm__
            // Trigger a system reset request
            SCB->AIRCR = 0x05FA0004;
#endif
        }
};



void SerialCLI::registerCommonCommands(void)
{
    registerCommand(new HelpCLICommand());
    registerCommand(new ResetCLICommand());
}

void SerialCLI::listCommands(void)
{
    CLICommand *cmd;

    m_serial.println("Commands (nargs)");
    for (cmd = (CLICommand *)m_commands.head(); cmd;
         cmd = (CLICommand *)m_commands.next()) {
        m_serial.print(cmd->command());
        m_serial.print(" (");
        m_serial.print(cmd->nargs());
        m_serial.println(")");
    }
}

void SerialCLI::handleInput(void)
{
    if (!m_connected) {
        if (!connect()) {
            return;
        }
    } else {
        if (!m_serial) {
            m_connected = false;
            return;
        }
    }

    while (m_serial.available()) {
        uint8_t ch = m_serial.read();
        uint8_t done = 0;

        if (ch == '\b') {
            if (m_index > 0) {
                m_index--;
                m_serial.print("\b \b");
            }
            continue;
        }

        if (m_index < SERIAL_BUFFER_SIZE - 1) {
            if (ch == '\n') {
                ch = '\0';
            }
            m_buffer[m_index++] = ch;
        } else {
            m_buffer[SERIAL_BUFFER_SIZE - 1] = '\0';
        }

        if (!ch) {
            parseBuffer();
            m_index = 0;
        }
    }
}


// vim:ts=4:sw=4:ai:et:si:sts=4
