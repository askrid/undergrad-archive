import java.util.Scanner;

public class StudentIDValidator {
    public static void main(String[] args) {
        Scanner sc = new Scanner(System.in);
        String studentID = sc.next();
        
        // validate student ID until the input is "exit"
        while (studentID.compareTo("exit") != 0) {
            validStudentID(studentID);
            
            studentID = sc.next();
        }

        sc.close();
    }

    private static boolean validStudentID(String studentID) {
        if (!isProperLength(studentID)) {
            System.out.println("The input length should be 10.");
            return false;
        } else if (!hasProperDivision(studentID)) {
            System.out.println("Fifth character should be '-'.");
            return false;
        } else if (!hasProperDigits(studentID)) {
            System.out.println("Contains an invalid digit.");
            return false;
        }
        
        // valid student ID
        System.out.println(studentID + " " + "is a valid student ID.");
        return true;
    }

    // check if student ID has length of 10
    private static boolean isProperLength(String studentID) {
        if (studentID.length() != 10)
            return false;
        return true;
    }

    // check if 5th char of student ID is '-'
    private static boolean hasProperDivision(String studentID) {
        if (studentID.charAt(4) != '-')
            return false;
        return true;
    }

    // check if all chars but 5th are digits
    private static boolean hasProperDigits(String studentID) {
        for (int i=0; i<studentID.length(); i++) {
            if (i == 4) continue;
            if (!Character.isDigit(studentID.charAt(i)))
                return false;
        }
        return true;
    }
}
