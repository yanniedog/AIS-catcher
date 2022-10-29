/*
	Copyright(c) 2021-2022 jvde.github@gmail.com

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once
#include <list>
#include <thread>
#include <mutex>

#ifdef HASCURL
#include <curl/curl.h>
#endif

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netdb.h>
#define SOCKET		int
#define closesocket close
#endif

#ifdef __ANDROID__
#include <netinet/in.h>
#endif

#include "Stream.h"
#include "AIS.h"
#include "Property.h"
#include "TCP.h"
#include "Utilities.h"

namespace IO {

	class HTTP : public PropertyToJSON, public Setting {

#ifdef HASCURL

		std::thread run_thread;
		bool terminate = false, running = false;
		std::mutex queue_mutex;
		std::string url = "";
		int INTERVAL = 30;
		int TIMEOUT = 10;
		std::string stationid;
		std::string model;
		std::string receiver;
		bool gzip = false;

		std::string response;

		enum class PROTOCOL{ DEFAULT, APRS } protocol = PROTOCOL::DEFAULT;


		static size_t curl_cb(void* contents, size_t size, size_t nmemb, std::string* s) {
			size_t len = size * nmemb;
			try {
				s->append((char*)contents, len);
			}
			catch (std::bad_alloc& e) {
				return 0;
			}
			return len;
		}

		int send(const std::string&);
		void post();
		void process();

	private:
		std::list<std::string> queue;

	public:
		~HTTP() {
			stopServer();
		}
#endif
	public:
		void Ready() {
			const std::lock_guard<std::mutex> lock(queue_mutex);
			queue.push_back(json);
		}

		virtual void Set(std::string option, std::string arg) {

#ifdef HASCURL
			Util::Convert::toUpper(option);

			if (option == "URL") {
				url = arg;
			}
			else if (option == "STATIONID" || option == "ID") {
				stationid = arg;
			}
			else if (option == "INTERVAL") {
				INTERVAL = Util::Parse::Integer(arg, 1, 60 * 60 * 24);
			}
			else if (option == "TIMEOUT") {
				TIMEOUT = Util::Parse::Integer(arg, 1, 30);
			}
			else if (option == "MODEL") {
				model = arg;
			}
			else if (option == "RECEIVER") {
				receiver = arg;
			}
			else if (option == "PROTOCOL") {
				Util::Convert::toUpper(arg);
				if (arg == "DEFAULT") {
					setMap(JSON_DICT_FULL);
					protocol = PROTOCOL::DEFAULT;
				}
				else if (arg == "APRS") {
					setMap(JSON_DICT_APRS);
					protocol = PROTOCOL::APRS;
				}
				else
					throw "HTTP: error - unknown protocol";
			}
			/*
			else if (option == "GZIP") {
				Util::Convert::toUpper(arg);
				gzip = Util::Parse::Switch(arg);
			}
			*/
			else
				throw "HTTP: Invalid setting.";
#else
			throw "HTTP: not implemented, please recompile with libcurl support.";
#endif
		}

		void startServer() {
#ifdef HASCURL
			if (!running) {
				run_thread = std::thread(&HTTP::process, this);
				running = true;
				std::cerr << "HTTP: start server." << std::endl;
			}
#else
			throw "HTTP: not implemented, please recompile with libcurl support.";
#endif
		}

		void stopServer() {
#ifdef HASCURL
			if (running) {
				running = false;
				terminate = true;
				run_thread.join();
				std::cerr << "HTTP: stop server." << std::endl;
			}
#endif
		}
	};

	class UDPEndPoint {
		std::string address;
		std::string port;

		int sourceID = -1;

	public:
		friend class UDP;
		friend class TCP;

		UDPEndPoint(std::string a, std::string p, int id = -1) {
			address = a, port = p;
			sourceID = id;
		}
		int ID() { return sourceID; }
	};

	class UDP : public StreamIn<AIS::Message> {
		SOCKET sock = -1;
		struct addrinfo* address = NULL;

	public:
		~UDP();
		UDP();

		void Receive(const AIS::Message* data, int len, TAG& tag);
		void openConnection(const std::string& host, const std::string& port);
		void openConnection(UDPEndPoint& u) { openConnection(u.address, u.port); }
		void closeConnection();
	};

	class TCP : public StreamIn<AIS::Message> {
		::TCP::Client con;

	public:
		~TCP();
		TCP();

		void Receive(const AIS::Message* data, int len, TAG& tag);
		void openConnection(const std::string& host, const std::string& port);
		void openConnection(UDPEndPoint& u) { openConnection(u.address, u.port); }
		void closeConnection();
	};

}
