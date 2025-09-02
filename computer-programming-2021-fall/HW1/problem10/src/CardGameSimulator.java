public class CardGameSimulator {
	private static final Player[] players = new Player[2];

	public static void simulateCardGame(String inputA, String inputB) {
		// DO NOT change the skeleton code.
		// You can add codes anywhere you want.

		players[0] = new Player("A", parseCards(inputA));
		players[1] = new Player("B", parseCards(inputB));

		Player turn = players[0];
		Player opp = players[1];
		Card curr = turn.getLargest();

		while (true) {
			if (curr == null) {
				printWinMessage(opp);
				break;
			}

			turn.playCard(curr);

			opp = turn;
			turn = (turn == players[0]) ? players[1] : players[0];
			curr = turn.getNext(curr);
		}
	}

	private static Card[] parseCards(String input) {
		String[] cards = input.split(" ");

		int number;
		char shape;
		Card card;
		Card [] deck = new Card[20];

		for (String e : cards) {
			number = Integer.parseInt(e.substring(0, 1));
			shape = e.charAt(1);
			card = new Card(number, shape);
			deck[card.getIndex()] = card;
		}

		return deck;
	}

	private static void printWinMessage(Player player) {
		System.out.printf("Player %s wins the game!\n", player);
	}
}

class Player {
	private String name;
	private Card[] deck;

	public Player(String name, Card[] deck) {
		this.name = name;
		this.deck = deck;
	}

	public void playCard(Card card) {
		deck[card.getIndex()] = null;
		System.out.printf("Player %s: %s\n", name, card);
	}

	public Card getLargest() {
		for (int i = 19; i >= 0; i--) {
			if (deck[i] != null) {
				return deck[i];
			}
		}
		return null;
	}

	public Card getLargest(char shape) {
		int i = 18;
		if (shape == 'X') {
			i++;
		}

		for (; i >= 0; i -= 2) {
			if (deck[i] != null) {
				return deck[i];
			}
		}
		return null;
	}

	public Card getNext(Card card) {
		int num = 2 * card.getNumber() + (card.getShape() == 'O' ? 1 : 0);
		if (deck[num] != null) {
			return deck[num];
		} else {
			return getLargest(card.getShape());
		}
	}

	@Override
	public String toString() {
		return name;
	}
}

class Card {
	private int number;
	private char shape;
	private int cardIndex;

	public Card(int number, char shape) {
		this.number = number;
		this.shape = shape;
		this.cardIndex = 2 * number + ((shape == 'O') ? 0 : 1);
	}

	public int getNumber() {
		return number;
	}

	public char getShape() {
		return shape;
	}

	public int getIndex() {
		return cardIndex;
	}

	@Override
	public String toString() {
		return "" + number + shape;
	}
}
