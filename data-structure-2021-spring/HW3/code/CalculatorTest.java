import java.io.*;
import java.util.EmptyStackException;
import java.util.Stack;
import java.util.ArrayList;

public class CalculatorTest {
	public static void main(String args[]) {
		BufferedReader br = new BufferedReader(new InputStreamReader(System.in));

		while (true) {
			try {
				String input = br.readLine();
				if (input.compareTo("q") == 0)
					break;

				command(input);
			} catch (Exception e) {
				System.out.println("ERROR");
			}
		}
	}

	private static void command(String input) throws Exception {
		ArrayList<Object> infix = parse(input);

		ArrayList<Object> postfix = getPostfix(infix);

		// calculate(): postfix 계산.
		Long result = calculate(postfix);

		// postfix 식과 계산 결과를 터미널에 출력.
		printExpression(postfix);
		System.out.println(result);
	}

	private static ArrayList<Object> parse(String input) throws Exception {
		input = input.replaceAll("\\s+", " ");
		
		for (int i=2; i < input.length(); i++) {
			if (Character.isDigit(input.charAt(i-2)) && Character.isDigit(input.charAt(i)) && input.charAt(i-1) == ' ') {
				throw new Exception("두 개 이상의 숫자 연속으로 나옴.");
			}
		}

		input = input.replaceAll(" ", "");
		ArrayList<Object> infix = new ArrayList<>();
		boolean prevOperand = false;

		for (int i=0; i < input.length(); i++) {
			Character ch = input.charAt(i);

			if (isOperator(ch)) {
				if (!prevOperand && ch.equals('-')) {
					ch = '~';
				} else if (!prevOperand) {
					throw new Exception("피연산자 부족.");
				}
				prevOperand = false;
				infix.add(ch);
			} else if (ch.equals('(')) {
				if (prevOperand) {
					throw new Exception("\'(\' 앞에 피연산자 있음.");
				}
				prevOperand = false;
				infix.add(ch);
			} else if (ch.equals(')')) {
				if (!prevOperand) {
					throw new Exception("\')\' 앞에 피연산자 없음.");
				}
				prevOperand = true;
				infix.add(ch);
			} else if (Character.isDigit(ch)) {
				Long num = (long) Character.getNumericValue(ch);

				if (prevOperand && infix.get(infix.size()-1).equals(')')) {
					throw new Exception("\')\' 뒤에 연산자 있음.");
				} else if (prevOperand) {
					num += 10 * (long) infix.remove(infix.size()-1);
				}
				prevOperand = true;
				infix.add(num);
			} else {
				throw new Exception("지원하지 않는 문자 입력.");
			}
		}

		if (!prevOperand) {
			throw new Exception("식이 피연산자로 끝나지 않음.");
		}

		return infix;
	}

	private static ArrayList<Object> getPostfix(ArrayList<Object> infix) throws Exception {
		// operators: postfix을 완성하는데 쓰이는 스택.
		Stack<Character> operators = new Stack<>();
		ArrayList<Object> postfix = new ArrayList<>();

		for (int i=0; i < infix.size(); i++) {
			Object o = infix.get(i);

			if (o instanceof Character) {
				Character ch = (char) o;
				if (isOperator(ch)) {
					// ch가 left-associative 연산자인 경우에는
					// ch가 operators의 top의 연산자보다 priority가 작거나 같으면, '(' 전까지 우선순위가 높은 연산자들을 모두 postfix에 넣는다.
					// ch가 right-associative 연산자인 경우에는, '^'(priority=3) 바로 다음 연산자로 '~'(priority=2)가 나오는 경우가 없고,
					// 모든 right-associative 연산자는 모든 left-associated 연산자보다 priority가 높기 때문에 바로 operators에 넣는다.
					if (!(o.equals('^') || o.equals('~'))) {
						while (!operators.empty() && operators.peek() != '(' && priority((char) o) <= priority(operators.peek())) {
							postfix.add((operators.pop()));
						}
					}
					operators.push(ch);
				} else if (ch.equals('(')) {
					// '('는 바로 operators에 넣고, ')'를 기다린다.
					operators.push(ch);
				} else if (ch.equals(')')) {
					// operators의 '(' 전까지의 모든 연산자들을 pop해서 postfix에 push한다.
					try {
						while (operators.peek() != '(') {
							postfix.add(operators.pop());
						}
						operators.pop();
					} catch (EmptyStackException e) {		// 짝이 되는 '('이 없는 경우 오류 발생.
						throw new Exception("괄호 개수 맞지 않음.");
					}
				}
			} else if (o instanceof Long) {
				Long num = (long) o;
				// 숫자는 바로 postfix에 넣는다.	
				postfix.add(num);
			}
		}

		// operators에 남은 연산자는 모두 postfix에 넣는다.
		while (!operators.empty()) {
			if (operators.peek().equals('(')) {
				throw new Exception("괄호 개수 맞지 않음.");
			}
			postfix.add(operators.pop());
		}

		return postfix;
	}

	private static int priority(Character operator) {
		switch ((char) operator) {
			case '^':
				return 3;
			case '~':
				return 2;
			case '*':
			case '/':
			case '%':
				return 1;
			case '+':
			case '-':
				return 0;
			default:
				return -1;
		}
	}

	private static boolean isOperator(Character ch) {
		if (priority(ch) == -1) {
			return false;
		} else {
			return true;
		}
	}

	private static Long calculate(ArrayList<Object> postfix) throws Exception {
		Stack<Long> result = new Stack<>();
		
		// postfix의 각 요소에 대해서
		// 숫자인 경우 result에 넣어서 계산되기를 기다리고, 연산자인 경우 result의 숫자 1~2개를 꺼내서 알맞은 연산을 한다.
		for (int i=0; i < postfix.size(); i++) {
			Object o = postfix.get(i);
			if (o instanceof Long) {
				result.push((Long) o);
			} else if ((char) o == '~') {
				result.push(operate((Character) o, result.pop(), (long) 0));
			} else {
				result.push(operate((Character) o, result.pop(), result.pop()));
			}
		}

		return result.pop();
	}

	private static Long operate(Character operator, Long operandR, Long operandL) throws Exception {
		Long result = (long) 0;

		try {
			switch ((char) operator) {
				case '^':
					if (operandL.equals((long) 0) && operandR.compareTo((long) 0) < 0) {	// 0 ^ (음수)의 경우 오류 발생.
						throw new ArithmeticException();
					}
					result = (long) Math.pow((double) operandL, (double) operandR);
					break;
				case '~':
					result = -operandR;
					break;
				case '*':
					result = operandL * operandR;
					break;
				case '/':
					result = operandL / operandR;
					break;
				case '%':
					result = operandL % operandR;
					break;
				case '+':
					result = operandL + operandR;
					break;
				case '-':
					result = operandL - operandR;
					break;
			}
		} catch (ArithmeticException e) {				// 0으로 나누는 산술 오류 발생.
			throw new Exception();
		}

		return result;
	}

	private static void printExpression(ArrayList<Object> expression) {
		for (int i=0; i < expression.size()-1; i++) {
			System.out.print(expression.get(i)+" ");
		}
		System.out.println(expression.get(expression.size()-1));
	}
}