import java.util.ArrayList;

// HashTable의 구현은 수업 자료를 참고하였다.
// 실제로 쓰이는 HashTable<T, K>에서 T는 String, K는 Coordinate다.
// 내부적으로 AVLTree<T, K>와 LinkedList<K>가 쓰인다는 것을 가정한다.
// 삭제 작업은 일어나지 않기 때문에 구현하지 않았다.

public class HashTable<T extends Comparable<T>, K extends Comparable<K>> {
	private final ArrayList<AVLTree<T, K>> table;
	private final int tableSize;

	// 실제로는 n = 100이 쓰인다.
	public HashTable(int n) {
		table = new ArrayList<AVLTree<T, K>>(n);
		tableSize = n;
		for (int i = 0; i < n; i++) {
			table.add(i, null);
		}
	}

	// Substring의 key(ASCII 코드의 합)은 외부에서 계산된 후에 인자로 넘겨진다.
	private int hash(int key) {
		return key % tableSize;
	}

	// key만 이용해서 검색하면, AVLTree를 반환한다. '@' 명령에 이용된다.
	// 값이 없으면 null을 반환한다.
	public AVLTree<T, K> searchTree(int key) {
		int slot = hash(key);
		return table.get(slot);
	}

	// key와 item 둘 다 이용해서 검색하면, AVLNode를 반환한다. '?' 명령에 이용된다.
	// slot이 비어있는 경우, 또는 해당 node가 없는 경우 null을 반환한다.
	public AVLNode<T, K> searchNode(int key, T item) {
		int slot = hash(key);
		if (table.get(slot) == null) {
			return null;
		} else {
			return table.get(slot).search(item);
		}
	}

	public void insert(int key, T item, K e) {
		int slot = hash(key);
		if (table.get(slot) == null) {
			table.set(slot, new AVLTree<T, K>());
		}
		table.get(slot).insert(item, e);
	}

	public void clear() {
		table.clear();
		for (int i = 0; i < tableSize; i++) {
			table.add(i, null);
		}
	}

	public boolean isEmpty() {
		for (AVLTree<T, K> e : table) {
			if (e != null) {
				return false;
			}
		}
		return true;
	}
}