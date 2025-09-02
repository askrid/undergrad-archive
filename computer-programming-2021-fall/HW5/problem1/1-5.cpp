
bool bibimbap_change(int* bills, int N) {
    int balance[] = {0, 0, 0};

    for (int i = 0; i < N; i++) {
        switch (bills[i]) {
            case 5:
                balance[0]++;
                break;
            case 10:
                if (balance[0] > 0) {
                    balance[0]--;
                    balance[1]++;
                } else return false;
                break;
            case 20:
                if (balance[0] > 0 && balance[1] > 0) {
                    balance[0]--;
                    balance[1]--;
                    balance[2]++;
                } else if (balance[0] >= 3) {
                    balance[0] -= 3;
                    balance[2]++;
                } else return false;
                break;
            default:
                break;
        }
    }

    return true;
}
