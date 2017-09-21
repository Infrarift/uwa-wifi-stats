/*
CITS2002 Project 1 2017
Name(s):             Jakrin Juangbhanich
Student number(s):   21789272
Date:                22/9/2017
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <ctype.h>

// Define commonly used values.
#define UNKNOWN_VENDOR "UNKNOWN-VENDOR"
#define UNKNOWN_ADDRESS "??:??:??"
#define BROADCAST_ADDRESS "ff:ff:ff:ff:ff:ff"
#define TEMP_COPY_FILE_NAME "_tempCopy.txt"
#define TRANSMIT_TOKEN 't'
#define RECEIVE_TOKEN 'r'
#define TAB_TOKEN "\t"
#define NULL_CHAR '\0'

// Define max count and length of vendors, names, and output.
#define V_COUNT 25000
#define V_NAME_LENGTH 100
#define V_ADDRESS_LENGTH 20
#define OUTPUT_COUNT 500
#define OUTPUT_LENGTH 200

extern char** environ;
char* process_name;

void ExitWithMessage(char* exit_msg)
{
	/*
	 * Function: Exits the app with failure and print a simple message to the console.
	 * ------------------------------------------------------------------
	 * exit_msg: The message to print.
	 */
	printf("%s\n", exit_msg);
	exit(EXIT_FAILURE);
}

void ExitWithError(char* error_msg, char* msg_param)
{
	/*
	 * Function: Exits the app with a fatal error, and prints a message, with a parameter to the console.
	 * ------------------------------------------------------------------
	 * error_msg: The main error message to print.
	 * msg_param: An additional string to include as part of the error message.
	 */
	char formatted_error[500];
	sprintf(formatted_error, "%s: %s %s", process_name, error_msg, msg_param);
	perror(formatted_error);
	exit(EXIT_FAILURE);
}

void Split(char* str, const char* delimiter, char* result[])
{
	/*
	* Function: Splits a string on a delimter, and put the elements into an array.
	* ------------------------------------------------------------------
	* str: The string to split.
	* delimiter: The character upon which the string is split.
	* result: An array of *char to write the results to.
	*/
	int i = 0;
	char* token = strtok(str, delimiter);
	while (token != NULL)
	{
		result[i] = token;
		token = strtok(NULL, TAB_TOKEN);
		i++;
	}
}

bool Compare(char* string1, char* string2)
{
	/*
	* Function: Wraps strcmp, compares if two strings are the same.
	* ------------------------------------------------------------------
	* string1: The first string to compare.
	* string2: The second string to compare.
	* 
	* returns bool: true if the strings are the same, otherwise false.
	*/
	return strcmp(string1, string2) == 0 ? true : false;
}

char* ConvertToVendorAddress(char* src_address, char* dst_address, bool reformat)
{
	/*
	* Function: Truncates a MAC address to find the vendor specific part of the address.
	*			Can additionally format it so that it can be compared to the addresses in the OUI file.
	* ------------------------------------------------------------------
	* src_address: The full MAC address to process.
	* dst_address: The destination string to put the vendor address into.
	* reformat: Whether to reformat the address for OUI comparison.
	*
	* returns char*: The vendor's part of the MAC address.
	*/
	for (int j = 0; j < 8; j++)
	{
		if (reformat)
			dst_address[j] = src_address[j] == ':' ? '-' : toupper(src_address[j]);
		else
			dst_address[j] = src_address[j];
	}
	dst_address[8] = NULL_CHAR;
	return dst_address;
}

int GetVendorIndex(char* address, int vendor_count, char vendor_addresses[][V_ADDRESS_LENGTH])
{
	/*
	* Function: Search for the index of a given address, in an array of known vendors.
	* ------------------------------------------------------------------
	* address: The full MAC address to search for.
	* vendor_count: The number of known vendors.
	* vendor_addresses: An array of known vendor addresses.
	*
	* returns int: The index of the vendor (if found), otherwise -1.
	*/
	char partial_address[9];
	ConvertToVendorAddress(address, partial_address, true);

	for (int i = 0; i < vendor_count; i++)
	{
		if (Compare(partial_address, vendor_addresses[i]))
			return i;
	}
	return -1;
}

char* GetVendorName(char* address, int vendor_count, char vendor_addresses[][V_ADDRESS_LENGTH], char vendor_names[][V_NAME_LENGTH])
{
	/*
	* Function: Finds the matching vendor name for the given MAC address if known.
	* ------------------------------------------------------------------
	* address: The full MAC address to identify.
	* vendor_count: The number of known vendors.
	* vendor_addresses: An array of strings, of all known vendors.
	* vendor_names: A matching array of known vendor names.
	*
	* returns char*: The name of the vendor if known, otherwise UNKNOWN-VENDOR.
	*/
	int address_index = GetVendorIndex(address, vendor_count, vendor_addresses);
	return address_index == -1 ? UNKNOWN_VENDOR : vendor_names[address_index];
}

void Trim(char* str)
{
	/*
	* Function: Terminates a string by replace all new-lines with a null char.
	* ------------------------------------------------------------------
	* str: The string to trim.
	*/
	int i = 0;
	while (str[i] != NULL_CHAR)
	{
		if (str[i] == '\n' || str[i] == '\r')
		{
			str[i] = NULL_CHAR;
			break;
		}
		i++;
	}
}

void CopyOutputToFile(char full_output[][OUTPUT_LENGTH])
{
	/*
	* Function: Write the entire output to a file.
	* ------------------------------------------------------------------
	* full_output: An array of strings to write to a file.
	*/
	FILE* file = fopen(TEMP_COPY_FILE_NAME, "w");
	if (file == NULL)
		ExitWithError("unable to create a temp file", TEMP_COPY_FILE_NAME);

	int i = 0;
	while (*full_output[i] != NULL_CHAR)
		fputs(full_output[i++], file);
	fclose(file);
}

void CallExternalProcess(char* process_name, char* exe_argv[])
{
	/*
	* Function: Fork this process and invoke an external program.
	* ------------------------------------------------------------------
	* process_name: The name/path of the program to invoke.
	* exe_argv: Input arguements to invoke the program with.
	*/
	int pid = fork();
	if (pid < 0)
		ExitWithError("unable to fork process", process_name);

	if (pid == 0)
	{
		execve(process_name, exe_argv, environ);
		ExitWithError("unable to invoke process", process_name);
	}

	if (pid > 0)
	{
		int status = 0;
		wait(&status);
	}
}

void SortResults(int vendor_count)
{
	/*
	* Function: Sort the output and print to console.
	* ------------------------------------------------------------------
	* vendor_count: The number of known vendors. WIll be used to determine sorting type.
	*/
	if (vendor_count == 0)
	{
		// No vendor names: Sort by package size in descending numerical order. 
		char* exe_argv[] = {"sort", TEMP_COPY_FILE_NAME, "-k2nr", NULL};
		CallExternalProcess("/usr/bin/sort", exe_argv);
	}
	else
	{
		// With vendor name: Sort by package size first, then by vendor name on alphabetical order.
		char* exe_argv[] = {"sort", "-t\t", "-k3nr", "-k2", TEMP_COPY_FILE_NAME, NULL};
		CallExternalProcess("/usr/bin/sort", exe_argv);
	}
}

void DeleteTempFile()
{
	/*
	* Function: Delete the temporary file created for sorting purposes.
	*/
	char* exe_argv[] = {"rm", TEMP_COPY_FILE_NAME, NULL};
	CallExternalProcess("/bin/rm", exe_argv);
}

int ExtractVendorInfo(char* file_name, char vendor_addresses[][V_ADDRESS_LENGTH], char vendor_names[][V_NAME_LENGTH])
{
	/*
	* Function: Process the OUI file and extract the vendor addresses and names to two respective arrays.
	* ------------------------------------------------------------------
	* file_name: The OUI file containing a list of known vendor name and address pairs.
	* vendor_addresses: The array to put the vendor addresses into.
	* vendor_names: The array to put the vendor names into.
	*
	* returns int: The number of vendors extracted.
	*/
	FILE* file = fopen(file_name, "r");
	if (file == NULL)
		ExitWithError("unable to open OUI file", file_name);

	char line[OUTPUT_LENGTH];
	int i = 0;

	while (fgets(line, sizeof line, file) != NULL)
	{
		strcpy(vendor_addresses[i], strtok(line, TAB_TOKEN));
		strcpy(vendor_names[i], strtok(NULL, TAB_TOKEN));
		Trim(vendor_names[i]);
		Trim(vendor_addresses[i]);
		i++;
	}
	return i;
}

char* CopyStringToOutput(char* copy_str, char* output_ptr)
{
	/*
	* Function: Helper utility. Appends a string into the final output.
	* ------------------------------------------------------------------
	* copy_str: The string to copy.
	* output_ptr: The current pointer of the full output string.
	*
	* returns char*: The current pointer position of the output string.
	*/
	int j = 0;
	while (copy_str[j] != NULL_CHAR)
	{
		*output_ptr = copy_str[j];
		output_ptr++;
		j++;
	}
	return output_ptr;
}

char* AddTabToOutput(char* output_ptr)
{
	/*
	* Function: Helper utility. Appends a tab token into the final output.
	* ------------------------------------------------------------------
	* output_ptr: The current pointer of the full output string.
	*
	* returns char*: The current pointer position of the output string.
	*/
	*output_ptr = '\t';
	output_ptr++;
	return output_ptr;
}

void AddLineEnding(char* output_pointer)
{
	/*
	* Function: Helper utility. Appends a newline and null char into the final output.
	* ------------------------------------------------------------------
	* output_ptr: The current pointer of the full output string.
	*
	* returns char*: The current pointer position of the output string.
	*/
	*output_pointer++ = '\n';
	*output_pointer = NULL_CHAR;
}

void ProcessSimplePackage(char* packet_size, char* address, char* output_ptr)
{
	/*
	* Function: Format a single line of a package, without vendor names.
	* ------------------------------------------------------------------
	* packet_size: The size of the packet.
	* address: The MAC address of the device.
	* output_ptr: The current pointer of the full output string.
	*/
	output_ptr = CopyStringToOutput(address, output_ptr);
	output_ptr = AddTabToOutput(output_ptr);
	output_ptr = CopyStringToOutput(packet_size, output_ptr);
	AddLineEnding(output_ptr);
}

void ProcessNamedPackage(char* packet_size, char* vendor_name, char vendor_address[9], char* output_ptr)
{
	/*
	* Function: Format a single line of a package, with vendor name.
	* ------------------------------------------------------------------
	* packet_size: The size of the packet.
	* vendor_name: The name of the device's vendor.
	* vendor_address: The (partial) address of the device.
	* output_ptr: The current pointer of the full output string.
	*/
	output_ptr = CopyStringToOutput(vendor_address, output_ptr);
	output_ptr = AddTabToOutput(output_ptr);
	output_ptr = CopyStringToOutput(vendor_name, output_ptr);
	output_ptr = AddTabToOutput(output_ptr);
	output_ptr = CopyStringToOutput(packet_size, output_ptr);
	AddLineEnding(output_ptr);
}

void AddToOutputAddress(char* address, int packet_size, char output_addresses[][V_ADDRESS_LENGTH], int output_packets[])
{
	/*
	* Function: Process a new packet-info line, either adding it to an existing one, or appending a new one to the array.
	* ------------------------------------------------------------------
	* address: The full MAC address of the device.
	* packet_size: The size of the packet.
	* output_addresses: An array of already processed MAC addresses.
	* output_packets: A matching array of already processed packet sizes.
	*/
	for (int i = 0; i < OUTPUT_COUNT; i++)
	{
		char* known_address = output_addresses[i];
		if (*known_address == NULL_CHAR)
		{
			strcpy(output_addresses[i], address);
			output_packets[i] += packet_size;
			break;
		}

		if (Compare(known_address, address))
		{
			output_packets[i] += packet_size;
			break;
		}
	}
}

void SumInputPackets(char* file_name, char device_type, int vendor_count, char vendor_addresses[][V_ADDRESS_LENGTH], char output_addresses[][V_ADDRESS_LENGTH], int output_packets[])
{
	/*
	* Function: Process the packets file, and sum all the packet sizes of the same device into one row of output.
	* ------------------------------------------------------------------
	* file_name: The file of the packets to read from.
	* device_type: Either r or t, to show the stats of the receiver or transmitter, respectively.
	* vendor_count: The number of known vendors.
	* vendor_addresses: An array of all known vendor addresses.
	* output_addresses: An array of already processed MAC addresses.
	* output_packets: A matching array of already processed packet sizes.
	*/

	// Initialize all packet values to default.
	for (int i = 0; i < OUTPUT_COUNT; i++)
	{
		output_addresses[i][0] = NULL_CHAR;
		output_packets[i] = 0;
	}

	// Read each line in the file, and sum the packet sizes on the vendor address.
	FILE* file = fopen(file_name, "r");
	if (file == NULL)
	{
		ExitWithError("unable to open packets file", file_name);
	}

	char line[OUTPUT_LENGTH];
	while (fgets(line, sizeof line, file) != NULL)
	{
		char* array[4];
		Split(line, TAB_TOKEN, array);
		char* transmit_address = array[1];
		char* receive_address = array[2];
		char* package_size = array[3];

		// Ignore broadcast addresses.
		if (Compare(receive_address, BROADCAST_ADDRESS))
			continue;

		char* address = device_type == TRANSMIT_TOKEN ? transmit_address : receive_address;

		// If we have vendor names, use the vendor address. Otherwise use the full address.
		if (vendor_count != 0)
		{
			char partial_address[9];
			int vendor_index = GetVendorIndex(address, vendor_count, vendor_addresses);
			address = vendor_index == -1 ? UNKNOWN_ADDRESS : ConvertToVendorAddress(address, partial_address, false);
		}
		AddToOutputAddress(address, atoi(package_size), output_addresses, output_packets);
	}
}

int ProcessInputFile(
	char output[][OUTPUT_LENGTH],
	int vendor_count,
	char vendor_addresses[][V_ADDRESS_LENGTH],
	char vendor_names[][V_NAME_LENGTH],
	char output_addresses[][V_ADDRESS_LENGTH],
	int output_packets[])
{
	/*
	* Function: Read all the lines from the input file. Format those lines to spec, and then output it into the target array.
	* ------------------------------------------------------------------
	* output: An array of strings, representing the full output of the program.
	* vendor_count: The number of known vendors.
	* vendor_addresses: An array of strings, of all known vendors.
	* vendor_names: A matching array of known vendor names.
	* output_addresses: An array of the MAC addresses to output.
	* output_packets: A matching array of the packet sizes to output.
	* 
	* returns int: The number of lines of the final output.
	*/
	int line_index = 0;
	while (*output_addresses[line_index] != NULL_CHAR)
	{
		char* address = output_addresses[line_index];
		char* vendor_name = GetVendorName(address, vendor_count, vendor_addresses, vendor_names);
		char buffer[20];
		sprintf(buffer, "%d", output_packets[line_index]);
		char* output_pointer = output[line_index];

		if (vendor_count == 0)
			ProcessSimplePackage(buffer, address, output_pointer);
		else
			ProcessNamedPackage(buffer, vendor_name, address, output_pointer);

		line_index++;
	}
	return line_index;
}

void CheckArguments(int argc, char* argv[])
{
	/*
	* Function: Checks if the input arguements are valid.
	*			If it is wrong, the program will exit with a failure.
	* ------------------------------------------------------------------
	* argc: Argument count supplied to the program.
	* argv: List of argument strings supplied to the program.
	*/
	if ((argc != 3 && argc != 4) || (argv[1][0] != RECEIVE_TOKEN && argv[1][0] != TRANSMIT_TOKEN))
	{
		char format_usage[250];
		sprintf(format_usage, "Usage: %s t|r packetsFile [OUIfile]", process_name);
		ExitWithMessage(format_usage);
	}
}

int main(int argc, char* argv[])
{
	process_name = argv[0];
	CheckArguments(argc, argv);

	// Get vendor name information.
	char vendor_addresses[V_COUNT][V_ADDRESS_LENGTH];
	char vendor_names[V_COUNT][V_NAME_LENGTH];
	int vendor_count = 0;

	// Declare arrays for all the output information.
	char output_mac[OUTPUT_COUNT][V_ADDRESS_LENGTH];
	int output_packet[OUTPUT_COUNT];
	char output_lines[OUTPUT_COUNT][OUTPUT_LENGTH];

	// If the 4th argv is provided, then extract the vendor name from the OUI file.
	if (argc == 4)
		vendor_count = ExtractVendorInfo(argv[3], vendor_addresses, vendor_names);

	// Default invocation: The 1st arg is the transmit type (t or r) and the 2nd arg is the input file to read.
	if (argc >= 3)
	{
		char* input_file = argv[2];
		char device_type = argv[1][0];

		SumInputPackets(input_file, device_type, vendor_count, vendor_addresses, output_mac, output_packet);
		ProcessInputFile(output_lines, vendor_count, vendor_addresses, vendor_names, output_mac, output_packet);
		CopyOutputToFile(output_lines);
		SortResults(vendor_count);
		DeleteTempFile();
	}
	return EXIT_SUCCESS;
}
