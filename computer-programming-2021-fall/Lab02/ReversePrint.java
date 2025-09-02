import java.util.Scanner;

public class ReversePrint {
    public static void main(String[] args) {
        Scanner scanner = new Scanner(System.in);

        // n = number of iteration = size of the String array
        int n = scanner.nextInt();
        String[] arr = new String[n];

        // get input strings and put them into the array
        for (int i=0; i<n; i++) {
            arr[i] = scanner.next();
        }
        
        scanner.close();

        // print the strings of the array
        for (String e : arr) {
            System.out.print(e + " ");
        }
        System.out.println();

        // print the strings in the reverse order
        for (int i=n-1; i>-1; i--) {
            System.out.print(arr[i] + " ");
        }
        System.out.println();
    }
}