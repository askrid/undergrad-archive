import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.util.regex.Pattern;
import java.util.regex.Matcher;

public class BigInteger
{
    public static final String QUIT_COMMAND = "quit";
    public static final String MSG_INVALID_INPUT = "입력이 잘못되었습니다.";
    public static final Pattern EXPRESSION_PATTERN = Pattern.compile("^\\s*[+\\-]?\\s*\\d+\\s*[+\\*\\-]\\s*[+\\-]?\\s*\\d+\\s*$");
    public static final int ARRAY_SIZE = 210;

    private boolean sign;
    private int length;
    private byte[] numArray;
  
    public BigInteger(boolean b, String s) {
        sign = b;
        length = s.length();
        numArray = new byte[ARRAY_SIZE];
        for (int i=0; i < length; i++) {
            int n = s.charAt(length-1-i) - '0';
            numArray[i] = (byte) n;
        }
    }
    private BigInteger(boolean b, byte[] arr, int n) {
        sign = b;
        length = n;
        numArray = new byte[ARRAY_SIZE];
        for (int i=0; i < length; i++) {
            numArray[i] = arr[i];
        }
    }
  
    public BigInteger add(BigInteger big) {
        BigInteger result;
        BigInteger invertedThis = new BigInteger(!this.sign, this.numArray, this.length);
        BigInteger invertedBig = new BigInteger(!big.sign, big.numArray, big.length);
        if (!this.sign && !big.sign) {
            byte[] newNumArr = new byte[ARRAY_SIZE];
            int newLength = (this.length > big.length) ? this.length : big.length;
            int carry = 0;
            for (int i=0; i < newLength+1; i++) {
                int n = carry + this.numArray[i] + big.numArray[i];
                int digit = n % 10;
                newNumArr[i] = (byte) digit;
                carry = n / 10;
            }
            if (newNumArr[newLength] == 1) {
                newLength++;
            }
            result = new BigInteger(false, newNumArr, newLength);
        } else if (!this.sign && big.sign) {
            result = this.subtract(invertedBig);
        } else if (this.sign && !big.sign) {
            result = big.subtract(invertedThis);
        } else {
            result = invertedThis.add(invertedBig);
            result.sign = true;
        }
        if (result.isZero()) {
            result.sign = false;
        }
        return result;
    }

    public BigInteger subtract(BigInteger big) {
        BigInteger result;
        BigInteger invertedThis = new BigInteger(!this.sign, this.numArray, this.length);
        BigInteger invertedBig = new BigInteger(!big.sign, big.numArray, big.length);
        if (!this.sign && !big.sign) {
            if (this.compareAbsoluteValue(big) >= 0) {
                byte[] newNumArr = new byte[ARRAY_SIZE];
                int newLength = 1;
                int tmpLength = (this.length > big.length) ? this.length : big.length;
                int carry = 0;
                for (int i=0; i < tmpLength; i++) {
                    int n = carry + (10+this.numArray[i]) - big.numArray[i];
                    int digit = n % 10;
                    newNumArr[i] = (byte) digit;
                    carry = (n / 10) - 1;
                    if (newNumArr[i] != 0) {
                        newLength = i + 1;
                    }
                }
                result = new BigInteger(false, newNumArr, newLength);
            } else {
                result = big.subtract(this);
                result.sign = true;
            }
        } else if (!this.sign && big.sign) {
            result = this.add(invertedBig);
        } else if (this.sign && !big.sign) {
            result = invertedThis.add(big);
            result.sign = true;
        } else {
            result = invertedBig.subtract(invertedThis);
        }
        if (result.isZero()) {
            result.sign = false;
        }
        return result;
    }

    public BigInteger multiply(BigInteger big) {
        BigInteger result;
        result = new BigInteger(false, "0");
        if (this.compareAbsoluteValue(big) >= 0) {
            BigInteger[] bigArr = new BigInteger[10];
            for (int i=0; i < 10; i++) {
                bigArr[i] = new BigInteger(false, "0");
                for (int j=0; j < i; j++) {
                    bigArr[i] = bigArr[i].add(this);
                }
                bigArr[i].sign = false;
            }
            for (int i=0; i < big.length; i++) {
                BigInteger tmp = new BigInteger(false, "0");
                tmp = tmp.add(bigArr[big.numArray[i]]);
                for (int j=0; j < i; j++) {
                    tmp.multTen();
                }
                result = result.add(tmp);
            }
        } else {
            result = big.multiply(this);
        }
        if (this.sign == big.sign || result.isZero()) {
            result.sign = false;
        } else {
            result.sign = true;
        }
        return result;
    }

    public int compareAbsoluteValue(BigInteger big) {
        if (this.length == big.length) {
            for (int i=this.length-1; i >= 0; i--) {
                if (this.numArray[i] == big.numArray[i]) {
                    continue;
                } else if (this.numArray[i] > big.numArray[i]) {
                    return 1;
                } else {
                    return -1;
                }
            }
            return 0;
        } else if (this.length > big.length) {
            return 1;
        } else {
            return -1;
        }
    }

    public boolean isZero() {
        return (this.length == 1) && (this.numArray[0] == 0);
    }

    public void multTen() {
        for (int i=this.length-1; i >= 0; i--) {
            this.numArray[i+1] = this.numArray[i];
        }
        this.numArray[0] = 0;
        this.length++;
    }
    
    @Override
    public String toString() {
        String minus = (sign) ? "-" : "";
        char[] charArray = new char[length];
        for (int i=0; i < length; i++) {
            charArray[i] = (char) (numArray[length-1-i] + '0');
        }
        String digits = new String(charArray);
        return minus + digits;
    }
  
    static BigInteger evaluate(String input) throws IllegalArgumentException {
        Matcher matcher = EXPRESSION_PATTERN.matcher(input);
        if (!matcher.find()) {
            throw new IllegalArgumentException();
        }
        input = input.replace(" ", "");
        int inputLength = input.length();
        int operatorIndex = 0;
        char operator = '+';
        for (int i=1; i < inputLength; i++) {
            char curr = input.charAt(i);
            char prev = input.charAt(i-1);
            if (Character.isDigit(prev) && (curr == '+' || curr == '-' || curr == '*')) {
                operatorIndex = i;
                operator = curr;
            }
        }
        boolean big1Sign = (input.charAt(0) == '-') ? true : false;
        boolean big2Sign = (input.charAt(operatorIndex+1) == '-') ? true : false;
        int big1StartIndex = (big1Sign || input.charAt(0) == '+') ? 1 : 0;
        int big2StartIndex = (big2Sign || input.charAt(operatorIndex+1) == '+') ? operatorIndex + 2 : operatorIndex + 1;
        BigInteger big1 = new BigInteger(big1Sign, input.substring(big1StartIndex, operatorIndex));
        BigInteger big2 = new BigInteger(big2Sign, input.substring(big2StartIndex, inputLength));
        switch (operator) {
            case '+':
                return big1.add(big2);
            case '-':
                return big1.subtract(big2);
            case '*':
                return big1.multiply(big2);
            default:
                return new BigInteger(true, "1");
        }
    }

    public static void main(String[] args) throws Exception
    {
        try (InputStreamReader isr = new InputStreamReader(System.in))
        {
            try (BufferedReader reader = new BufferedReader(isr))
            {
                boolean done = false;
                while (!done)
                {
                    String input = reader.readLine();
                    try
                    {
                        done = processInput(input);
                    }
                    catch (IllegalArgumentException e)
                    {
                        System.err.println(MSG_INVALID_INPUT);
                    }
                }
            }
        }
    }
  
    static boolean processInput(String input) throws IllegalArgumentException
    {
        boolean quit = isQuitCmd(input);
        if (quit)
        {
            return true;
        }
        else
        {
            BigInteger result = evaluate(input);
            System.out.println(result.toString());
            return false;
        }
    }
  
    static boolean isQuitCmd(String input)
    {
        return input.equalsIgnoreCase(QUIT_COMMAND);
    }
}