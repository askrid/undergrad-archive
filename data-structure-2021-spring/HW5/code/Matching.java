import java.io.*;
import java.util.LinkedList;
import java.util.Queue;

public class Matching
{
	private static final HashTable<String, Coordinate> database = new HashTable<String, Coordinate>(100);
	public static void main(String args[]) {
		BufferedReader br = new BufferedReader(new InputStreamReader(System.in));

		while (true) {
			try {
				String input = br.readLine();
				if (input.compareTo("QUIT") == 0)
					break;

				command(input);
			} catch (IOException e) {
				System.out.println("입력이 잘못되었습니다. 오류 : " + e.toString());
			}
		}
	}

	private static void command(String input) throws IOException
	{
		char commandType = input.charAt(0);
		String commandInput = input.substring(2);
		switch (commandType) {
			case '<':
				database.clear();
				getFile(commandInput);
				break;
			case '@':
				printData(commandInput);
				break;
			case '?':
				searchPattern(commandInput);
				break;
			default:
				throw new IOException("잘못된 명령어");
		}
	}

	private static void getFile(String input) throws IOException{
		try {
			File file = new File(input);
			FileReader fr = new FileReader(file);
			BufferedReader br = new BufferedReader(fr);
			String line = "";

			int lineNum = 1;
			while ((line = br.readLine()) != null) {
				putString(line, lineNum++);
			}
			br.close();
		} catch (FileNotFoundException e) {
			throw new IOException("파일 없음");
		} catch (IOException e) {
			throw e;
		}
	}
	
	private static void putString(String line, int x) {
		final int K = 6;

		for (int y = 1; y <= line.length()-K+1; y++) {
			String sub = line.substring(y-1, y-1+K);
			Coordinate coor = new Coordinate(x, y);
			int key = getKey(sub);
			database.insert(key, sub, coor);
		}
	}

	// 문자열의 각 문자의 ASCII 코드를 합한 값을 반환한다.
	private static int getKey(String str) {
		int res = 0;
		for (char ch : str.toCharArray()) {
			res += (int) ch;
		}
		return res;
	}

	private static void printData(String input) throws IOException{
		int key = 0;
		try {
			key = Integer.parseInt(input);
		} catch (NumberFormatException e) {
			throw new IOException("@ 다음에 숫자가 오지 않음");
		}

		AVLTree<String, Coordinate> tree = database.searchTree(key);

		// 해당 slot이 비어있는 경우.
		if (tree == null) {
			System.out.println("EMPTY");
			return;
		}

		// tree가 null이 아니면, 노드가 적어도 하나는 있다.
		Queue<AVLNode<String, Coordinate>> queue = tree.preorder();
		AVLNode<String, Coordinate> prev = null;

		for (AVLNode<String, Coordinate> e : queue) {
			if (prev != null) {
				System.out.print(" ");
			}
			System.out.print(e.item);
			prev = e;
		}
		System.out.println();
	}

	private static void searchPattern(String input) {
		LinkedList<Coordinate> coordList = null;

		// 패턴 문자열을 크기 6의 여러 문자열로 나눈 다음, coordList에 남아있는 좌표와의 비교를 통해 완전히 일치하는 문자열의 위치를 구한다.
		for (int i = 0; i < input.length(); i += 6) {
			if (i + 6 > input.length()) {
				i = input.length() - 6;
			}
			String sub = input.substring(i, i + 6);
			AVLNode<String, Coordinate> node = database.searchNode(getKey(sub), sub);
			if (node == null) {
				System.out.println("(0, 0)");
				return;
			}
			if (i == 0) {
				coordList = (LinkedList<Coordinate>) node.list.clone();	// 원본 LinkedList의 훼손 방지 위해 clone() 이용.
			} else {
				sieve(coordList, node.list, i);
			}
		}

		// 과제 제출 후 수정한 부분. coordList의 원소가 모두 걸러진 결과, 아무것도 남게 되지 않은 경우를 놓쳤다.
		if (coordList.size() == 0) {
			System.out.println("(0, 0)");
			return;
		}

		Coordinate prev = null;

		for (Coordinate e : coordList) {
			if (prev != null) {
				System.out.print(" ");
			}
			System.out.print(e.toString());
			prev = e;
		}
		System.out.println();
	}

	// 글자의 위치가 diff만큼 차이나는 Coordinate만 남기고, 나머지는 제거한다.
	private static void sieve(LinkedList<Coordinate> coordList, LinkedList<Coordinate> compareList, int diff) {
		LinkedList<Coordinate> tmp = (LinkedList<Coordinate>) coordList.clone();
		for (Coordinate e : tmp) {
			boolean match = false;
			for (Coordinate f : compareList) {
				if ((e.x == f.x) && (f.y - e.y == diff)) {
					match = true;
				}
			}
			if (!match) {
				coordList.remove(e);
			}
		}
	}
}