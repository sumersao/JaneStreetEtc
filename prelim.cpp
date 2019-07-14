/* HOW TO RUN
   1) Configure things in the Configuration class
   2) Compile: g++ -o bot.exe bot.cpp
   3) Run in loop: while true; do ./bot.exe; sleep 1; done
*/

/* C includes for networking things */
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

/* C++ includes */
#include <string>
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <vector>
#include <sstream>
#include <map>


using namespace std;

/* The Configuration class is used to tell the bot how to connect
   to the appropriate exchange. The `test_exchange_index` variable
   only changes the Configuration when `test_mode` is set to `true`.
*/
class Configuration {
private:
  /*
    0 = prod-like
    1 = slower
    2 = empty
  */
  static int const test_exchange_index = 1;
public:
  std::string team_name;
  std::string exchange_hostname;
  int exchange_port;
  /* replace REPLACEME with your team name! */
  Configuration(bool test_mode) : team_name("liberalartseducation"){
    exchange_port = 20000; /* Default text based port */
    if(true == test_mode) {
      exchange_hostname = "test-exch-" + team_name;
      exchange_port += test_exchange_index;
    } else {
      exchange_hostname = "production";
    }
  }
};

/* Connection establishes a read/write connection to the exchange,
   and facilitates communication with it */
class Connection {
private:
  FILE * in;
  FILE * out;
  int socket_fd;
public:
  Connection(Configuration configuration){
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
      throw std::runtime_error("Could not create socket");
    }
    std::string hostname = configuration.exchange_hostname;
    hostent *record = gethostbyname(hostname.c_str());
    if(!record) {
      throw std::invalid_argument("Could not resolve host '" + hostname + "'");
    }
    in_addr *address = reinterpret_cast<in_addr *>(record->h_addr);
    std::string ip_address = inet_ntoa(*address);
    struct sockaddr_in server;
    server.sin_addr.s_addr = inet_addr(ip_address.c_str());
    server.sin_family = AF_INET;
    server.sin_port = htons(configuration.exchange_port);

    int res = connect(sock, ((struct sockaddr *) &server), sizeof(server));
    if (res < 0) {
      throw std::runtime_error("could not connect");
    }
    FILE *exchange_in = fdopen(sock, "r");
    if (exchange_in == NULL){
      throw std::runtime_error("could not open socket for writing");
    }
    FILE *exchange_out = fdopen(sock, "w");
    if (exchange_out == NULL){
      throw std::runtime_error("could not open socket for reading");
    }

    setlinebuf(exchange_in);
    setlinebuf(exchange_out);
    this->in = exchange_in;
    this->out = exchange_out;
    this->socket_fd = res;
  }

  /** Send a string to the server */
  void send_to_exchange(std::string input) {
    std::string line(input);
    /* All messages must always be uppercase */
    std::transform(line.begin(), line.end(), line.begin(), ::toupper);
    int res = fprintf(this->out, "%s\n", line.c_str());
    if (res < 0) {
      throw std::runtime_error("error sending to exchange");
    }
  }

  /** Read a line from the server, dropping the newline at the end */
  std::string read_from_exchange()
  {
    /* We assume that no message from the exchange is longer
       than 10,000 chars */
    const size_t len = 10000;
    char buf[len];
    if(!fgets(buf, len, this->in)){
      throw std::runtime_error("reading line from socket");
    }

    int read_length = strlen(buf);
    std::string result(buf);
    /* Chop off the newline */
    result.resize(result.length() - 1);
    return result;
  }
};

/** Join a vector of strings together, with a separator in-between
    each string. This is useful for space-separating things */
std::string join(std::string sep, std::vector<std::string> strs) {
  std::ostringstream stream;
  const int size = strs.size();
  for(int i = 0; i < size; ++i) {
    stream << strs[i];
    if(i != (strs.size() - 1)) {
      stream << sep;
    }
  }
  return stream.str();
}

//returns index of element in a list
int getind(vector<string>& a, string elem) {
  for(int i = 0; i < a.size(); i++) {
    if(a[i] == elem) return i;
  }
}

int getmed(vector<pair<int, int> >& a, int tot){
  if(a.size() == 0) return 0;
  int rsum = 0;
  for(int i = 0; i < a.size(); i++){
    if(rsum >= tot/2) {
      return a[i].first;
    }
    rsum += a[i].second;
  }

}

int main(int argc, char *argv[])
{
  ios_base::sync_with_stdio(false);
  cin.tie(0);

  vector<string> securids;
  securids.push_back("BOND");
  securids.push_back("VALBZ");
  securids.push_back("VALE");
  securids.push_back("GS");
  securids.push_back("MS");
  securids.push_back("WFC");
  securids.push_back("XLF");

  vector<pair<int, int> > lastids;
  vector<double> lastFV;
  vector<pair<int, int> >SMA;
  vector<int> bookreads;
  for(int i = 0; i < 7; i++){
    lastids.push_back(make_pair(0, 0));
    lastFV.push_back(0.0);
    bookreads.push_back(0);
    SMA.push_back(make_pair(0,0));
  }

  vector<int> amt;
  amt.push_back(100);
  amt.push_back(10);
  amt.push_back(10);
  amt.push_back(100);
  amt.push_back(100);
  amt.push_back(100);
  amt.push_back(100);

    // Be very careful with this boolean! It switches between test and prod
  bool test_mode = true;
  Configuration config(test_mode);
  Connection conn(config);

  map<string, double> fair_value_map;
  fair_value_map["VALBZ"] = 0.0;
  fair_value_map["VALE"] = 0.0;
  fair_value_map["GS"] = 0.0;
  fair_value_map["MS"] = 0.0;
  fair_value_map["WFC"] = 0.0;
  fair_value_map["XLF"] = 0.0;

  map<string, double> real_fair_value_map;
  real_fair_value_map["BOND"] = 1000;

  std::vector<std::string> data;
  data.push_back(std::string("HELLO"));
  data.push_back(config.team_name);
    /*
      A common mistake people make is to conn.send_to_exchange() > 1
      time for every conn.read_from_exchange() response.
      Since many write messages generate marketdata, this will cause an
      exponential explosion in pending messages. Please, don't do that!
    */
  conn.send_to_exchange(join(" ", data));
  string line = conn.read_from_exchange();
  cout << "The exchange replied: " << line << endl;

  int ids = 1;
  int Nsize = 5;
  double smoother = 2.0/(1.0 + 1.0*Nsize);
    //start our trading here
  while(1) {
      //read from the exchange
    string curline = conn.read_from_exchange();
    vector<string> res;
    istringstream iss(curline);
    for(string curline; iss >> curline;) {
      res.push_back(curline);
      // cout << curline << " ";
    }
    // cout << endl;

    

    if(curline.find("BOOK") == 0) {

      bookreads[curind]++;
      //type is res[1]"
      for(int i = 0; i < res.size(); i++){
        cout << res[i] << " ";
      }
      cout << endl;

      int curind = getind(securids, res[1]);
        //find location of sell
      int locsell = getind(res, "SELL");

      //here is buy
      int tot = 0;
      vector<pair<int, int> > low;
      for(int i = 3; i < locsell; i++) {
        string cur = res[i];
        replace(cur.begin(), cur.end(), ':', ' ');
        vector<int> array;
        stringstream ss(cur);
        int temp;
        while (ss >> temp) array.push_back(temp);

        low.push_back(make_pair(array[0], array[1]));
        tot += array[1];
      }

      //get median
      int temp1 = getmed(low, tot); 

      //here is sell
      tot = 0;
      vector<pair<int, int> > hi;
      for(int i = locsell+1; i < res.size(); i++) {
        string cur = res[i];
        replace(cur.begin(), cur.end(), ':', ' ');
        vector<int> array;
        stringstream ss(cur);
        int temp;
        while (ss >> temp) array.push_back(temp);

        hi.push_back(make_pair(array[0], array[1]));
        tot += array[1];

      }

      int temp2 = getmed(hi, tot); 

      int todays = (temp1 + temp2)/2;

      //we just need to build an SMA for now
      if(bookreads[curind] <= Nsize) {
        int sum = SMA[curind].first;
        int days = SMA[curind].second;
        SMA[curind] = make_pair(sum + todays, days + 1);
        fair_value_map[res[1]] = (double)((sum + todays)/(1.0*(days+1)));
      }
      else if(bookreads[curind] == Nsize + 1) {
        //now we slide with our SMA
        SMAval = SMA[curind].first/SMA.curind[second];
        fair_value_map[res[1]] = (todays - SMAval)*smoother + SMAval;
      } 
      else {
        //now we slide with our EMA
        EMAval = fair_value_map[res[1]];
        fair_value_map[res[1]] = (todays - EMAval)*smoother + EMAval;
      }

      if (res[1] == "VALE" || res[1] == "VALBZ") {
        if (fair_value_map["VALBZ"] != 0 && fair_value_map["VALE"] != 0) {
          real_fair_value_map["VALBZ"] = (1/3.0) * fair_value_map["VALE"] + (2/3.0) * fair_value_map["VALBZ"];
          real_fair_value_map["VALE"] = (1/3.0) * fair_value_map["VALE"] + (2/3.0) * fair_value_map["VALBZ"];
          // cout << "REAL FAIR VALUE OF VALBZ/VALE IS " << (1/3.0) * fair_value_map["VALE"] + (2/3.0) * fair_value_map["VALBZ"] << endl;
        }
      } else {
        if (fair_value_map["GS"] != 0 && fair_value_map["MS"] != 0 && fair_value_map["WFC"] != 0) {
          real_fair_value_map["XLF"] = (3 * fair_value_map["BOND"] + 2 * fair_value_map["GS"] + 3 * fair_value_map["MS"] + 2 * fair_value_map["WFC"])/10.0;
          // cout << "REAL FAIR VALUE OF XLF IS " << real_fair_value_map["XLF"] << endl;
        }
      }

      int fairval = int(fair_value_map[res[1]]);
      if (res[1] == "XLF" || res[1] == "VALBZ" || res[1] == "VALE") {
        fairval = int(real_fair_value_map[res[1]]);
      }

      // // cout << "REAL VALUE OF VALBZ IS " << real_fair_value_map["VALBZ"] << endl;
      // // cout << "REAL VALUE OF VALE IS " << real_fair_value_map["VALE"] << endl;
      // // cout << "MARKET VALUE OF VALBZ IS " << fair_value_map["VALBZ"] << endl;
      // // cout << "MARKET VALUE OF VALE IS " << fair_value_map["VALE"] << endl;
      // // cout << "REAL VALUE OF XLF IS " << real_fair_value_map["XLF"] << endl;
      // cout << lastFV[curind] << " " << fairval << endl;

      if(abs(fairval - lastFV[curind]) >= 3) {
        //cancel our last two orders
        conn.send_to_exchange("CANCEL " + to_string(lastids[curind].first));
        conn.send_to_exchange("CANCEL " + to_string(lastids[curind].second));

        cout << lastids[curind].first << " " << lastids[curind].second << endl;

        //update orders
        vector<string> buy;
        buy.push_back(string("ADD"));
        buy.push_back(to_string(ids+1));
        buy.push_back(res[1]);
        buy.push_back(string("BUY"));
        buy.push_back(to_string(int(fairval - 1)));
        buy.push_back(to_string(3));
        conn.send_to_exchange(join(" ", buy));

        for(int i = 0; i < buy.size(); i++){
          cout << buy[i] << " ";
        }
        cout << endl;

        vector<string> sell;
        sell.push_back(string("ADD"));
        sell.push_back(to_string(ids+2));
        sell.push_back(res[1]);
        sell.push_back(string("SELL"));
        sell.push_back(to_string(int(fairval + 1)));
        sell.push_back(to_string(3));
        conn.send_to_exchange(join(" ", sell));

        for(int i = 0; i < sell.size(); i++){
          cout << sell[i] << " ";
        }
        cout << endl;

        for(int i = 0; i < res.size(); i++){
          cout << res[i] << " ";
        }
        cout << endl;



        lastFV[curind] = fairval;
        lastids[curind] = make_pair(ids+1, ids+2);
        ids+=2;
      }
    }
    else if(curline.find("TRADE") == 0) {

    }
    else if(curline.find("OPEN") == 0) {

    }
    else{
          

    }


  }

  return 0;
}
