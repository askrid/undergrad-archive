
void merge_arrays(int* arr1, int len1, int* arr2, int len2) {
    int* tmp = new int[len1+len2];

    int i = 0, j = 0;
    while (i < len1 && j < len2) {
        if (arr1[i] <= arr2[j]) {
            tmp[i+j] = arr1[i];
            i++;
        } else {
            tmp[i+j] = arr2[j];
            j++;
        }
    }

    while (i < len1) {
        tmp[i+j] = arr1[i];
        i++;
    }
    while (j < len2) {
        tmp[i+j] = arr2[j];
        j++;
    }

    for (int k = 0; k < len1+len2; k++) {
        arr1[k] = tmp[k];
    }

    delete[] tmp;
}
