#include <iostream>
#include <boost/asio.hpp>
#include <boost/asio/ip/host_name.hpp>

using namespace std;
using namespace boost::asio;
using namespace boost::asio::ip;
int main() {
    try {
        io_service ios;
        tcp::resolver resolver(ios);
        tcp::resolver::query query(host_name(), "");
        tcp::resolver::iterator it = resolver.resolve(query);

        tcp::endpoint endpoint;
        while (it != tcp::resolver::iterator()) {
            endpoint = *it++;
            if (endpoint.address().is_v4() && !endpoint.address().is_loopback()) {
                cout << "Local IP address: " << endpoint.address().to_string() << endl;
                break;
            }
        }
    } catch (const std::exception &e) {
        cerr << "Error: " << e.what() << endl;
    }

    return 0;
}
