// x값은 줄 번호, y값은 시작 글자의 위치를 나타낸다.

public class Coordinate implements Comparable<Coordinate> {
	public final int x;
	public final int y;

	public Coordinate(int x, int y) {
		this.x = x;
		this.y = y;
	}

	public String toString() {
		String res = "(" + Integer.toString(x) + ", " + Integer.toString(y) + ")";
		
		return res;
	}

	public int compareTo(Coordinate other) {
		if (this.x > other.x) {
			return 1;
		} else if (this. x < other. x) {
			return -1;
		} else {
			if (this.y > other.y) {
				return 1;
			} else if (this.y < other.y) {
				return -1;
			} else {
				return 0;
			}
		}
	}
}