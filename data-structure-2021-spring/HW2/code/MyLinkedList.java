import java.util.Iterator;
import java.util.NoSuchElementException;

// 수업시간에 강의한 코드의 일부다. (시작)

public class MyLinkedList<T> implements ListInterface<T> {

	private final Node<T> head;
	private int numItems;

	public MyLinkedList() {
		head = new Node<>(null);
		numItems = 0;
	}

	public MyLinkedList(T obj) {
		head = new Node<>(obj);
		numItems = 0;
	}

    public final Iterator<T> iterator() {
    	return new MyLinkedListIterator<>(this);
    }

	@Override
	public boolean isEmpty() {
		return head.getNext() == null;
	}

	@Override
	public int size() {
		return numItems;
	}

	@Override
	public void append(T item) {
		add(numItems, item);
	}

	@Override
	public void add(int index, T item) {
		Node<T> prevNode = getNode(index-1);
		Node<T> newNode = new Node<>(item);
		newNode.setNext(prevNode.getNext());
		prevNode.setNext(newNode);
		numItems++;
	}

	@Override
	public T get(int index) {
		Node<T> currNode = getNode(index);
		return currNode.getItem();
	}

	@Override
	public T remove(int index) {
		Node<T> prevNode = getNode(index-1);
		Node<T> currNode = getNode(index);
		prevNode.setNext(currNode.getNext());
		numItems--;
		return currNode.getItem();
	}

	@Override
	public void removeAll() {
		head.setNext(null);
		numItems = 0;
	}

	public Node<T> getNode(int index) {
		if (index < -1 || index > numItems-1) {
			throw new IndexOutOfBoundsException();
		}
		Node<T> curr = head;
		for (int i=0; i <= index; i++) {
			curr = curr.getNext();
		}
		return curr;
	}
}

// 수업시간에 강의한 코드의 일부다. (끝)

class MyLinkedListIterator<T> implements Iterator<T> {

	private final MyLinkedList<T> list;
	private Node<T> currNode;
	private Node<T> prevNode;

	public MyLinkedListIterator(MyLinkedList<T> list) {
		this.list = list;
		this.currNode = list.getNode(-1);
		this.prevNode = null;
	}

	@Override
	public boolean hasNext() {
		return currNode.getNext() != null;
	}

	@Override
	public T next() {
		if (!hasNext())
			throw new NoSuchElementException();

		prevNode = currNode;
		currNode = currNode.getNext();

		return currNode.getItem();
	}

	@Override
	public void remove() {
		if (prevNode == null)
			throw new IllegalStateException("next() should be called first");
		if (currNode == null)
			throw new NoSuchElementException();
		int index = 0;
		Node<T> tempNode = list.getNode(index);
		for (; tempNode != currNode; index++) {
			tempNode = tempNode.getNext();
		}
		list.remove(index);
		currNode = prevNode;
		prevNode = null;
	}
}