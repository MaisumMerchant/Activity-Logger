#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/statvfs.h>
#include <curl/curl.h>

#define LOG_FILE "system_usage.log"
#define DROPBOX_TOKEN ""

// Function to extract all details from our system
void log_system_usage() {
	
	// Opening .log file
	FILE *log = fopen(LOG_FILE, "a");
	if (log == NULL) {
		perror("Failed to open log file");
		return;
	}

	// Printing Time
	time_t now = time(NULL);
	char *time_str = ctime(&now);
	time_str = strtok(time_str, "\n");
	fprintf(log, "[%s]\n", time_str);
	fprintf(log, "---------------------------------\n");

	// Function to convert ticks into hour minutes and seconds
	void ticks_to_hms(unsigned long ticks, char *buffer, size_t size) {
		double seconds = ticks * 0.01;
		unsigned long hours = (unsigned long) seconds / 3600;
		unsigned long minutes = ((unsigned long) seconds % 3600) / 60;
		unsigned long remaining_seconds = (unsigned long) seconds % 60;
		snprintf(buffer, size, "%02lu:%02lu:%02lu", hours, minutes, remaining_seconds);
	}

	// Getting CPU Usage Information using "stat" file
	FILE *cpu_file = fopen("/proc/stat", "r");
	if (cpu_file != NULL) {
		char buffer[256];
		char hms_buffer[16];
		fgets(buffer, sizeof(buffer), cpu_file);

		unsigned long user, nice, system, idle, iowait, irq, softirq, steal, guest, guest_nice;
		sscanf(buffer, "cpu  %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu", &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal, &guest, &guest_nice);
		fprintf(log, "CPU Usage:\n");
		ticks_to_hms(user, hms_buffer, sizeof(hms_buffer));
		fprintf(log, "    - User Time:       %lu ticks (%s)\n", user, hms_buffer);
		ticks_to_hms(nice, hms_buffer, sizeof(hms_buffer));
		fprintf(log, "    - Nice Time:       %lu ticks (%s)\n", nice, hms_buffer);
		ticks_to_hms(system, hms_buffer, sizeof(hms_buffer));
		fprintf(log, "    - System Time:     %lu ticks (%s)\n", system, hms_buffer);
		ticks_to_hms(idle, hms_buffer, sizeof(hms_buffer));
		fprintf(log, "    - Idle Time:       %lu ticks (%s)\n", idle, hms_buffer);
		ticks_to_hms(iowait, hms_buffer, sizeof(hms_buffer));
		fprintf(log, "    - I/O Wait Time:   %lu ticks (%s)\n", iowait, hms_buffer);
		ticks_to_hms(irq, hms_buffer, sizeof(hms_buffer));
		fprintf(log, "    - IRQ Time:        %lu ticks (%s)\n", irq, hms_buffer);
		ticks_to_hms(softirq, hms_buffer, sizeof(hms_buffer));
		fprintf(log, "    - SoftIRQ Time:    %lu ticks (%s)\n", softirq, hms_buffer);
		ticks_to_hms(steal, hms_buffer, sizeof(hms_buffer));
		fprintf(log, "    - Steal Time:      %lu ticks (%s)\n", steal, hms_buffer);
		ticks_to_hms(guest, hms_buffer, sizeof(hms_buffer));
		fprintf(log, "    - Guest Time:      %lu ticks (%s)\n", guest, hms_buffer);
		ticks_to_hms(guest_nice, hms_buffer, sizeof(hms_buffer));
		fprintf(log, "    - Guest Nice Time: %lu ticks (%s)\n", guest_nice, hms_buffer);
		fclose(cpu_file);
	}

	// Getting CPU Memory Information using "meminfo" file
	FILE *mem_file = fopen("/proc/meminfo", "r");
	if (mem_file != NULL) {
		unsigned long mem_total, mem_free;
		fscanf(mem_file, "MemTotal: %lu kB\nMemFree: %lu kB", &mem_total, &mem_free);
		fprintf(log, "Memory Usage:\n");
		fprintf(log, "    - Total Memory:   %lu MB\n", mem_total/1024);
		fprintf(log, "    - Free Memory:    %lu MB\n", mem_free/1024);
		fclose(mem_file);
	}

	// Getting CPU Disk Usage Information using "statvfs function" available in library "<sys/statvfs.h>"
	struct statvfs disk_info;
	if (statvfs("/", &disk_info) == 0) {
		unsigned long total_disk = disk_info.f_blocks * disk_info.f_frsize / (1024 * 1024);
		unsigned long free_disk = disk_info.f_bfree * disk_info.f_frsize / (1024 * 1024);
		unsigned long used_disk = total_disk - free_disk;
		fprintf(log, "Disk Usage:\n");
		fprintf(log, "    - Total Disk:     %lu GB\n", total_disk/1024);
		fprintf(log, "    - Used Disk:      %lu GB\n", used_disk/1024);
		fprintf(log, "    - Free Disk:      %lu GB\n", free_disk/1024);
	}

	// Getting Running Process Information using "loadavg" file
	FILE *proc_file = fopen("/proc/loadavg", "r");
	if (proc_file != NULL) {
		double load1, load5, load15;
		int running_tasks, total_tasks, last_pid;
		fscanf(proc_file, "%lf %lf %lf %d/%d %d", &load1, &load5, &load15, &running_tasks, &total_tasks, &last_pid);
		fprintf(log, "Process Info:\n");
		fprintf(log, "    - Load Averages:  %.2f (1 min), %.2f (5 min), %.2f (15 min)\n", load1, load5, load15);
		fprintf(log, "    - Running Tasks:  %d\n", running_tasks);
		fprintf(log, "    - Total Tasks:    %d\n", total_tasks);
		fprintf(log, "    - Last PID:       %d\n", last_pid);
		fclose(proc_file);
	}

	// Getting Running Process Information using "net/dev" file
	FILE *net_file = fopen("/proc/net/dev", "r");
	if (net_file != NULL) {
		char buffer[256];
		while (fgets(buffer, sizeof(buffer), net_file)) {
			if (strstr(buffer, "eth0:") || strstr(buffer, "wlan0:")) {
				char interface[16];
				unsigned long rx_bytes, tx_bytes;
				sscanf(buffer, "%15s %lu %*s %*s %*s %*s %*s %*s %*s %lu", interface, &rx_bytes, &tx_bytes);
				fprintf(log, "Network Usage (%s):\n", strtok(interface, ":"));
				fprintf(log, "    - Received: %lu B\n", rx_bytes);
				fprintf(log, "    - Sent:     %lu B\n", tx_bytes);
			}
		}
		fclose(net_file);
	}

	// Getting Running Process Information using "meminfo" file
	FILE *swap_file = fopen("/proc/meminfo", "r");
	if (swap_file != NULL) {
		char buffer[256];
		unsigned long swap_total = 0, swap_free = 0;
		while (fgets(buffer, sizeof(buffer), swap_file)) {
			if (sscanf(buffer, "SwapTotal: %lu kB", &swap_total) == 1 || sscanf(buffer, "SwapFree: %lu kB", &swap_free) == 1);
		}
		fprintf(log, "Swap Usage:\n");
		fprintf(log, "    - Total Swap:     %lu MB\n", swap_total/1024);
		fprintf(log, "    - Free Swap:      %lu MB\n", swap_free/1024);
		fclose(swap_file);
	}
	
	// Closing File
	fprintf(log, "---------------------------------\n\n");
	fclose(log);

}

void upload_to_dropbox(const char *file_path) {
	CURL *curl;
	CURLcode res;

	curl_global_init(CURL_GLOBAL_DEFAULT);
	curl = curl_easy_init();

	if (curl) {
		FILE *file = fopen(file_path, "rb");
		if (!file) {
			perror("Failed to open log file for uploading");
			return;
		}

		fseek(file, 0, SEEK_END);
		long file_size = ftell(file);
		rewind(file);

		char *buffer = malloc(file_size);
		fread(buffer, 1, file_size, file);

		struct curl_slist *headers = NULL;
		headers = curl_slist_append(headers, "Content-Type: application/octet-stream");

		char auth_header[256];
		snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", DROPBOX_TOKEN);
		headers = curl_slist_append(headers, auth_header);

		headers = curl_slist_append(headers, "Dropbox-API-Arg: {\"path\": \"/system_usage.log\",\"mode\": \"overwrite\"}");

		curl_easy_setopt(curl, CURLOPT_URL, "https://content.dropboxapi.com/2/files/upload");
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		curl_easy_setopt(curl, CURLOPT_POST, 1L);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, buffer);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, file_size);

		res = curl_easy_perform(curl);

		if (res != CURLE_OK) {
			fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
		} else {
			printf("Log file successfully uploaded to Dropbox.\n");
		}

		curl_easy_cleanup(curl);
		curl_slist_free_all(headers);
		free(buffer);
		fclose(file);
	}

	curl_global_cleanup();
}

int main() {
	while (1) {
		log_system_usage();
		upload_to_dropbox(LOG_FILE);
		sleep(60);
	}
	return 0;
}