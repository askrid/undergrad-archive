public interface ListInterface<T> extends Iterable<T> {
	boolean isEmpty();
	int size();
	void append(T item);
	void add(int index, T item);
	T get(int index);
	T remove(int index);
	void removeAll();
}