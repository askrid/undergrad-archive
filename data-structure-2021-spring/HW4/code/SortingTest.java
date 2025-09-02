import java.io.*;
import java.util.*;

public class SortingTest
{
	public static void main(String args[])
	{
		BufferedReader br = new BufferedReader(new InputStreamReader(System.in));

		try
		{
			boolean isRandom = false;	// 입력받은 배열이 난수인가 아닌가?
			int[] value;	// 입력 받을 숫자들의 배열
			String nums = br.readLine();	// 첫 줄을 입력 받음
			if (nums.charAt(0) == 'r')
			{
				// 난수일 경우
				isRandom = true;	// 난수임을 표시

				String[] nums_arg = nums.split(" ");

				int numsize = Integer.parseInt(nums_arg[1]);	// 총 갯수
				int rminimum = Integer.parseInt(nums_arg[2]);	// 최소값
				int rmaximum = Integer.parseInt(nums_arg[3]);	// 최대값

				Random rand = new Random();	// 난수 인스턴스를 생성한다.

				value = new int[numsize];	// 배열을 생성한다.
				for (int i = 0; i < value.length; i++)	// 각각의 배열에 난수를 생성하여 대입
					value[i] = rand.nextInt(rmaximum - rminimum + 1) + rminimum;
			}
			else
			{
				// 난수가 아닐 경우
				int numsize = Integer.parseInt(nums);

				value = new int[numsize];	// 배열을 생성한다.
				for (int i = 0; i < value.length; i++)	// 한줄씩 입력받아 배열원소로 대입
					value[i] = Integer.parseInt(br.readLine());
			}

			// 숫자 입력을 다 받았으므로 정렬 방법을 받아 그에 맞는 정렬을 수행한다.
			while (true)
			{
				int[] newvalue = (int[])value.clone();	// 원래 값의 보호를 위해 복사본을 생성한다.

				String command = br.readLine();

				long t = System.currentTimeMillis();
				switch (command.charAt(0))
				{
					case 'B':	// Bubble Sort
						newvalue = DoBubbleSort(newvalue);
						break;
					case 'I':	// Insertion Sort
						newvalue = DoInsertionSort(newvalue);
						break;
					case 'H':	// Heap Sort
						newvalue = DoHeapSort(newvalue);
						break;
					case 'M':	// Merge Sort
						newvalue = DoMergeSort(newvalue);
						break;
					case 'Q':	// Quick Sort
						newvalue = DoQuickSort(newvalue);
						break;
					case 'R':	// Radix Sort
						newvalue = DoRadixSort(newvalue);
						break;
					case 'X':
						return;	// 프로그램을 종료한다.
					default:
						throw new IOException("잘못된 정렬 방법을 입력했습니다.");
				}
				if (isRandom)
				{
					// 난수일 경우 수행시간을 출력한다.
					System.out.println((System.currentTimeMillis() - t) + " ms");
				}
				else
				{
					// 난수가 아닐 경우 정렬된 결과값을 출력한다.
					for (int i = 0; i < newvalue.length; i++)
					{
						System.out.println(newvalue[i]);
					}
				}

			}
		}
		catch (IOException e)
		{
			System.out.println("입력이 잘못되었습니다. 오류 : " + e.toString());
		}
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////
	private static int[] DoBubbleSort(int[] value)
	{
		for (int i=value.length-1; i>0; i--) {
			for (int j=0; j<i; j++) {
				if (value[j] > value[j+1]) {
					swap(value, j, j+1);
				}
			}
		}

		return (value);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////
	private static int[] DoInsertionSort(int[] value)
	{
		for (int i=1; i<value.length; i++) {
			for (int j=i; j>0; j--) {
				if (value[j] < value[j-1]) {
					swap(value, j, j-1);
				} else {
					break;
				}
			}
		}

		return (value);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////
	private static int[] DoHeapSort(int[] value)
	{
		Heap heapArr = new Heap(value);
		heapArr.buildHeap();
		try {
			for (int i=value.length-1; i>=0; i--) {
				value[i] = heapArr.deleteMax();
			}
		} catch (Exception e) {
			System.err.println("index error occured in deleteMax()");
		}

		return (value);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////
	private static int[] DoMergeSort(int[] value)
	{
		int[] res = value.clone();

		mergeSort(value, res, 0, value.length-1);
		
		return (res);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////
	private static int[] DoQuickSort(int[] value)
	{
		quickSort(value, 0, value.length-1);

		return (value);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////
	private static int[] DoRadixSort(int[] value)
	{
		final int INT_DIGITS = (int) Math.log10(Integer.MAX_VALUE) + 1;
		int size = value.length;
		int[] count = new int[19]; 						// -9 ~ 9
		int[] tmp = new int[size];

		for (int i=0; i<INT_DIGITS; i++) {
			for (int j=0; j<19; j++) {					// count 초기화
				count[j] = 0;
			}
			int num = (int) Math.pow(10.0, (double) i);	// num = 1, 10, 100, ...
			boolean quit = true;
			for (int j=0; j<size; j++) {
				int digit = (value[j] / num) % 10;
				count[digit+9]++;
				if (digit != 0) {						// 모든 digit이 0이면 반복문 종료
					quit = false;
				}
			}
			if (quit) {
				break;
			}
			for (int j=1; j<19; j++) {					// count의 누적합 저장
				count[j] += count[j-1];
			}
			for (int j=size-1; j>=0; j--) {				// 정렬된 결과를 tmp에 저장
				int digit = (value[j] / num) % 10;
				tmp[count[digit+9]-1] = value[j];
				count[digit+9]--;
			}
			value = tmp.clone();
		}

		return (value);
	}

	private static void swap(int[] arr, int p1, int p2) {
		int tmp = arr[p1];
		arr[p1] = arr[p2];
		arr[p2] = tmp;
	}

// Heap class 구현은 강의 노트를 참고하였음을 밝힌다.
	private static class Heap {
		
		private int size;
		int[] arr;

		private Heap(int[] value) {
			size = value.length;
			arr = new int[value.length];
			for (int i=0; i<value.length; i++) {
				arr[i] = value[i];
			}
		}

		public void buildHeap() {
			if (size >= 2) {
				for (int i=(size-2)/2; i>=0; i--) {
					percolateDown(i);
				}
			}
		}

		public int deleteMax() throws Exception{
			if(!isEmpty()) {
				int max = arr[0];
				arr[0] = arr[size-1];
				size--;
				percolateDown(0);
				return max;
			} else {
				throw new Exception();
			}
		}

		public boolean isEmpty() {
			return size == 0;
		}

		public void percolateDown(int i) {
			int child = 2*i + 1;
			int rightChild = 2*i + 2;
			if (child <= size-1) {
				if (rightChild <= size-1 && arr[rightChild] > arr[child]) {
					child = rightChild;
				}
				if (arr[child] > arr[i]) {
					swap(arr, child, i);
					percolateDown(child);
				}
			}
		}
	}

	private static void mergeSort(int[] value, int[] res, int p, int r) {
		int size = r - p + 1;

		// basecase
		if (size < 2) {
			return;
		}

		// division & recursion
		int mid = p + size / 2;
		mergeSort(res, value, p, mid-1);
		mergeSort(res, value, mid, r);

		// mergence
		int i = p;
		int j = mid;
		int k = p;
		while ((i<=mid-1) && (j<=r)) {
			if (value[i] < value[j]) {
				res[k++] = value[i++];
			} else {
				res[k++] = value[j++];
			}
		}
		while (i <= mid-1) {
			res[k++] = value[i++];
		}
		while (j <= r) {
			res[k++] = value[j++];
		}
	}

	private static void quickSort(int[] value, int p, int r) {
		int size = r - p + 1;

		// basecase
		if (size < 2) {
			return;
		}

		// partition
		// pivot = value[r]
		int pivotIdx = p;
		for (int i=p; i<r; i++) {
			if (value[i] < value[r]) {
				swap(value, i, pivotIdx++);
			}
		}
		swap(value, pivotIdx, r);

		// recursion
		quickSort(value, p, pivotIdx-1);
		quickSort(value, pivotIdx+1, r);
	}
}