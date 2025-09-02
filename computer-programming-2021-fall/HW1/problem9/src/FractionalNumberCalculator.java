import java.lang.Math;

public class FractionalNumberCalculator {
	public static void printCalculationResult(String equation) {
		// DO NOT change the skeleton code.
		// You can add codes anywhere you want.

		String[] input = equation.split(" ");
		char op = input[1].charAt(0);
		FractionalNumber n1 = parse(input[0]);
		FractionalNumber n2 = parse(input[2]);
		FractionalNumber res;

		if (op == '+') {
			res = n1.add(n2);
		} else if (op == '-') {
			res = n1.subtract(n2);
		} else if (op == '*') {
			res = n1.multiply(n2);
		} else if (op == '/') {
			res = n1.divide(n2);
		} else {
			res = n1;
		}

		System.out.println(res);
	}

	private static FractionalNumber parse(String input) {
		int numerator;
		int denominator = 1;
		int i = input.indexOf('/') + 1;

		if (input.contains("/")) {
			numerator = Integer.parseInt(input.substring(0, i-1));
			denominator = Integer.parseInt(input.substring(i));
		} else {
			numerator = Integer.parseInt(input);
		}

		return new FractionalNumber(numerator, denominator);
	}
}

class FractionalNumber {
	private int numerator;
	private int denominator;

	FractionalNumber(int numerator, int denominator) {
		this.numerator = numerator;
		this.denominator = denominator;
		this.reduce();
	}

	public FractionalNumber add(FractionalNumber other) {
		int commonDenominator = this.denominator * other.denominator / gcd(this.denominator, other.denominator);
		int newNumerator = this.getCommonNumerator(commonDenominator) + other.getCommonNumerator(commonDenominator);
		FractionalNumber n = new FractionalNumber(newNumerator, commonDenominator);
		n.reduce();
		return n;
	}

	public FractionalNumber subtract(FractionalNumber other) {
		FractionalNumber tmp = new FractionalNumber(-1*other.numerator, other.denominator);
		return this.add(tmp);
	}

	public FractionalNumber multiply(FractionalNumber other) {
		int newNumerator = this.numerator * other.numerator;
		int newDenominator = this.denominator * other.denominator;
		FractionalNumber n = new FractionalNumber(newNumerator, newDenominator);
		n.reduce();
		return n;
	}

	public FractionalNumber divide(FractionalNumber other) {
		FractionalNumber n = this.multiply(other.getInverse());
		n.reduce();
		return n;
	}

	private void reduce() {
		int gcd = gcd(Math.abs(numerator), Math.abs(denominator));
		numerator /= gcd;
		denominator /= gcd;
	}

	private int getCommonNumerator(int commonDenominator) {
		return numerator * commonDenominator / denominator;
	}

	private static int gcd(int a, int b) {
		if (b == 0) {
			return a;
		}
		return gcd(b, a % b);
	}

	private FractionalNumber getInverse() {
		int newNumerator = this.denominator;
		int newDenominator = Math.abs(this.numerator);

		if (this.numerator < 0) {
			newNumerator *= -1;
		}

		return new FractionalNumber(newNumerator, newDenominator);
	}

	public String toString() {
		if (denominator == 1) {
			return Integer.toString(numerator);
		}
		return numerator + "/" + denominator;
	}
}
