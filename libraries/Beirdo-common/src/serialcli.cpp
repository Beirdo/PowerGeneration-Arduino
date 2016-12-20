#include <Arduino.h>
#include <EEPROM.h>
#include <avr/eeprom.h>
#include <string.h>
#include "serialcli.h"

CLICommand::CLICommand(uint8_t *command, uint8_t nargs)
{
    m_command = command;
    m_len = strlen(command);
    m_nargs = nargs;
}

uint8_t CLICommand::compare(uint8_t *command)
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

SerialCLI cli;

SerialCLI::SerialCLI(void)
{
    m_commands = LinkedList();
    m_index = 0;
    registerCommonCommands();
}

void SerialCLI::initialize(void)
{
    Serial.println("CLI Ready");
    prompt();
}

void SerialCLI::registerCommand(CLICommand *command)
{
    m_commands.add((void *)command);
}

void SerialCLI::parseBuffer(void)
{
    uint8_t *ch;
    uint8_t *command;
    uint8_t nargs = 0;
    uint8_t *args[8];   // support a max of 8 args
    CLICommand *cmd;

    // Strip leading whitespace
    for (ch = m_buffer; *ch == ' ' || *ch == '\t'; ch++);

    // Blank line
    if (!*ch) {
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
    for (cmd = m_commands.head(); cmd; cmd = m_commands.next()) {
        if (cmd->compare(command)) {
            break;
        }
    }

    // Do it if we have a match
    if (cmd) {
        if (!cmd->pre_run(nargs)) {
            Serial.print("ERROR: Command ");
            Serial.print((char *)cmd->command());
            Serial.print(" requires ");
            Serial.print(cmd->nargs());
            Serial.println(" arguments");
        } else if (!cmd->run(nargs, args)) {
            Serial.println("ERROR: Command failed");
        } else {
            Serial.println("SUCCESS");
        }
    } else {
        Serial.println("ERROR: Unknown command");
    }
    prompt();
}

class HelpCLICommand : public CLICommand {
    public:
        HelpCLICommand(void) : CLICommand("help", 0) {};
        virtual uint8_t run(uint8_t nargs, uint8_t **args) 
          { cli.listCommands();  return(1); };
};

class ResetCLICommand : public CLICommand {
    public:
        ResetCLICommand(void) : CLICommand("reset", 0) {};
        virtual uint8_t run(uint8_t nargs, uint8_t **args) 
          { Serial.println("Resetting...");  delay(1000); asm volatile ("jmp 0"); };
};



void SerialCLI::registerCommonCommands(void)
{
    registerCommand(new HelpCLICommand());
    registerCommand(new ResetCLICommand());
}

void SerialCLI::listCommands(void)
{
    CLICommand *cmd;

    Serial.println("Commands (nargs)");
    for (cmd = m_commands.head(); cmd; cmd = m_commands.next()) {
        Serial.print((char *)cmd->command());
        Serial.print(" (");
        Serial.print(cmd->nargs());
        Serial.println(")");
    }
}

void SerialCLI::handleInput(void)
{
    while (Serial.available()) {
        uint8_t ch = Serial.read();
        uint8_t done = 0;

        if (ch == '\b') {
            if (m_index > 0) {
                m_index--;
                Serial.print("\b \b");
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
