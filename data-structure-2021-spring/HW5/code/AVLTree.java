import java.util.LinkedList;
import java.util.Queue;

// AVLTree<T>의 구현은 수업 자료를 참고하였다.
// 삭제 작업은 일어나지 않기 때문에 구현하지 않았다.

public class AVLTree<T extends Comparable<T>, K extends Comparable<K>> {
	private AVLNode<T, K> root;
	private final Queue<AVLNode<T, K>> nodeQue = new LinkedList<>();
	private static final int LL = 1, LR = 2, RR = 3, RL = 4, NO_NEED = 0, ILLEGAL = -1;
	
	public AVLTree() {
		this.root = null;
	}

	// 패턴 검색에서 사용된다.
	// 반환된 AVLNode<T, K>에는 해당되는 Substring의 모든 좌표가 LinkedList 형태로 저장되어있다.
	public AVLNode<T, K> search(T item) {
		return searchItem(root, item);
	}
	private AVLNode<T, K> searchItem(AVLNode<T, K> tNode, T item) {
		if (tNode == null) {
			return null;
		} else if (item.compareTo(tNode.item) == 0) {
			return tNode;
		} else if (item.compareTo(tNode.item) < 0) {
			return searchItem(tNode.left, item);
		} else {
			return searchItem(tNode.right, item);
		}
	}

	// insertion에는 파라미터로 Substring과 Coordinate가 모두 요구된다.
	// 이미 존재하는 Substring이 들어올 경우, 해당 AVLNode의 list에 Coordinate를 추가한다.
	public void insert(T item, K e) {
		root = insertItem(root, item, e);
	}
	private AVLNode<T, K> insertItem(AVLNode<T, K> tNode, T item, K e) {
		int type;
		if (tNode == null) {
			tNode = new AVLNode<T, K>(item);
			tNode.add(e);
		} else if (item.compareTo(tNode.item) < 0) {
			tNode.left = insertItem(tNode.left, item, e);
			tNode.height = 1 + Math.max(tNode.rightHeight(), tNode.leftHeight());
			type = needBalance(tNode);
			tNode = balanceAVL(tNode, type);
		} else if (item.compareTo(tNode.item) > 0) {
			tNode.right = insertItem(tNode.right, item, e);
			tNode.height = 1 + Math.max(tNode.rightHeight(), tNode.leftHeight());
			type = needBalance(tNode);
			tNode = balanceAVL(tNode, type);
		} else {
			tNode.add(e);
		}
		return tNode;
	}

	public Queue<AVLNode<T, K>> preorder() {
		nodeQue.clear();
		preorderTraversal(root);
		return nodeQue;
	}
	private void preorderTraversal(AVLNode<T, K> tNode) {
		if (tNode != null) {
			nodeQue.offer(tNode);
			preorderTraversal(tNode.left);
			preorderTraversal(tNode.right);
		}
	}

	private int needBalance(AVLNode<T, K> tNode) {
		int type = ILLEGAL;
		if (tNode.leftHeight() + 2 <= tNode.rightHeight()) {
			if (tNode.right.leftHeight() <= tNode.right.rightHeight()) {
				type = RR;
			} else {
				type = RL;
			}
		} else if (tNode.rightHeight() + 2 <= tNode.leftHeight()) {
			if (tNode.left.rightHeight() <= tNode.left.leftHeight()) {
				type = LL;
			} else {
				type = LR;
			}
		} else {
			type = NO_NEED;
		}
		return type;
	}

	private AVLNode<T, K> balanceAVL(AVLNode<T, K> tNode, int type) {
		AVLNode<T, K> res = null;
		switch (type) {
			case NO_NEED:
				res = tNode;
				break;
			case LL:
				res = rightRotate(tNode);
				break;
			case LR:
				tNode.left = leftRotate(tNode.left);
				res = rightRotate(tNode);
				break;
			case RR:
				res = leftRotate(tNode);
				break;
			case RL:
				tNode.right = rightRotate(tNode.right);
				res = leftRotate(tNode);
				break;
			default:
				System.err.println("balanceAVL(): invalid type");
				break;
		}
		return res;
	}

	private AVLNode<T, K> rightRotate(AVLNode<T, K> tNode) {
		AVLNode<T, K> LChild = tNode.left;
		AVLNode<T, K> LRChild = LChild.right;
		LChild.right = tNode;
		tNode.left = LRChild;
		tNode.height = 1 + Math.max(tNode.leftHeight(), tNode.rightHeight());
		LChild.height = 1 + Math.max(LChild.leftHeight(), LChild.rightHeight());
		return LChild;
	}	
	private AVLNode<T, K> leftRotate(AVLNode<T, K> tNode) {
		AVLNode<T, K> RChild = tNode.right;
		AVLNode<T, K> RLChild = RChild.left;
		RChild.left = tNode;
		tNode.right = RLChild;
		tNode.height = 1 + Math.max(tNode.leftHeight(), tNode.rightHeight());
		RChild.height = 1 + Math.max(RChild.leftHeight(), RChild.rightHeight());
		return RChild;
	}
}

// AVLNode의 구현은 수업 자료를 참고하였다.
// 실제로 쓰이는 AVLNode<T, K>에서 T는 String, K는 Coordinate다.
// 필드에는 Substring과 LinkedList<Coordinate>이 포함된다.

class AVLNode<T extends Comparable<T>, K extends Comparable<K>> {
	public final T item;
	public final LinkedList<K> list;
	public AVLNode<T, K> left, right;
	public int height;

	public AVLNode(T item) {
		this.item = item;
		this.list = new LinkedList<K>();
		this.left = this.right = null;
		this.height = 1;
	}

	public void add(K e) {
		this.list.add(e);
	}

	// sentinel 대신에 아래 두 메소드를 이용한다.
	// 오른쪽 또는 왼쪽 자식이 없을(null) 경우, height = 0을 반환한다.
	public int rightHeight() {
		if (this.right == null) {
			return 0;
		} else {
			return this.right.height;
		}
	}
	public int leftHeight() {
		if (this.left == null) {
			return 0;
		} else {
			return this.left.height;
		}
	}
}