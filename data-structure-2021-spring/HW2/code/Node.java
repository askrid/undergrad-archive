public class Node<T> {

    private T item;
    private Node<T> next;

    public Node(T obj) {
        this.item = obj;
        this.next = null;
    }
    
    public Node(T obj, Node<T> next) {
    	this.item = obj;
    	this.next = next;
    }
    
    public final T getItem() {
    	return item;
    }
    
    public Node<T> getNext() {
    	return this.next;
    }

    public void setItem(T obj) {
        item = obj;
    }

    public void setNext(Node<T> next) {
        this.next = next;
    }
}