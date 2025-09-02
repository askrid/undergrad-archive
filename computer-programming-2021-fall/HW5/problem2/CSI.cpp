#include "CSI.h"
#include <fstream>
#include <sstream>
#include <string>
#include <cmath>
#include <algorithm>

#define EPS 1e-10

Complex::Complex(): real(0), imag(0) {}

CSI::CSI(): data(nullptr), num_packets(0), num_channel(0), num_subcarrier(0) {}

CSI::~CSI() {
    if(data) {
        for(int i = 0 ; i < num_packets; i++) {
            delete[] data[i];
        }
        delete[] data;
    }
}

int CSI::packet_length() const {
    return num_channel * num_subcarrier;
}

void CSI::print(std::ostream& os) const {
    for (int i = 0; i < num_packets; i++) {
        for (int j = 0; j < packet_length(); j++) {
            os << data[i][j] << ' ';
        }
        os << std::endl;
    }
}

std::ostream& operator<<(std::ostream &os, const Complex &c) {
    std::string sign_imag = "";
    if (c.imag >= 0) sign_imag = '+';

    return os << c.real << sign_imag << c.imag << 'i';
}

void read_csi(const char* filename, CSI* csi) {
    std::ifstream file(filename);
    
    std::string line;
    std::getline(file, line);
    csi->num_packets = std::stoi(line);
    std::getline(file, line);
    csi->num_channel = std::stoi(line);
    std::getline(file, line);
    csi->num_subcarrier = std::stoi(line);

    csi->data = new Complex*[csi->num_packets];
    for (int packet = 0; packet < csi->num_packets; packet++) {
        csi->data[packet] = new Complex[csi->packet_length()];
        for (int sub = 0; sub < csi->num_subcarrier; sub++) {
            for (int chan = 0; chan < csi->num_channel; chan++) {
                Complex val;
                std::getline(file, line);
                val.real = std::stoi(line);
                std::getline(file, line);
                val.imag = std::stoi(line);

                csi->data[packet][chan * csi->num_subcarrier + sub] = val;
            }
        }
    }
}

double** decode_csi(CSI* csi) {
    double** decoded_data = new double*[csi->num_packets];
    for (int packet = 0; packet < csi->num_packets; packet++) {
        decoded_data[packet] = new double[csi->packet_length()];
        for (int i = 0; i < csi->packet_length(); i++) {
            int real = csi->data[packet][i].real;
            int imag = csi->data[packet][i].imag;
            decoded_data[packet][i] = sqrt(real*real + imag*imag);
        }
    }
    return decoded_data;
}

double* get_med(double** decoded_csi, int num_packets, int packet_length) {
    double* med_data = new double[num_packets];
    bool is_odd = packet_length % 2;

    for (int packet = 0; packet < num_packets; packet++) {
        double packet_data[packet_length];
        std::copy(decoded_csi[packet], decoded_csi[packet] + packet_length, packet_data);
        
        if (is_odd) {
            std::nth_element(packet_data, packet_data + packet_length/2, packet_data + packet_length);
            med_data[packet] = packet_data[packet_length/2];
        } else {
            std::nth_element(packet_data, packet_data + packet_length/2, packet_data + packet_length);
            std::nth_element(packet_data, packet_data + packet_length/2 - 1, packet_data + packet_length);
            med_data[packet] = (packet_data[packet_length/2] + packet_data[packet_length/2 - 1]) / 2.0;
        }
    }

    return med_data;
}

double breathing_interval(double** decoded_csi, int num_packets) {
    int sum_interval(0), prev_peak(-1), count_peak(0);
    double data[num_packets];

    for (int packet = 0; packet < num_packets; packet++) {
        data[packet] = decoded_csi[packet][0];
    }

    for (int packet = 0; packet < num_packets; packet++) {
        bool cond1 = (packet - 2 < 0) || data[packet] > data[packet - 2] + EPS;
        bool cond2 = (packet - 1 < 0) || data[packet] > data[packet - 1] + EPS;
        bool cond3 = (packet + 1 >= num_packets) || data[packet] > data[packet + 1] + EPS;
        bool cond4 = (packet + 2 >= num_packets) || data[packet] > data[packet + 2] + EPS;

        if (cond1 && cond2 && cond3 && cond4) {
            sum_interval += (count_peak) ? packet - prev_peak : 0;
            prev_peak = packet;
            count_peak++;
            packet += 2;
        }
    }

    if (count_peak < 2) return num_packets;
    return sum_interval / (count_peak - 1.0);
}
