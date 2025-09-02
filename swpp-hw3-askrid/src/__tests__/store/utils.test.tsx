import axios from "axios";
import { useCachedFetcher } from "../../store/utils";

describe("store utils", () => {
  describe("cached fetcher", () => {
    it("does not fetch duplicate data", () => {
      axios.get = jest.fn().mockResolvedValue({ data: { content: "hello!" } });
      const fetchData = useCachedFetcher();

      for (let i = 0; i < 20; i++) {
        fetchData("URL_1");
        fetchData("URL_2");
      }

      expect(axios.get).toBeCalledTimes(2);
    });
  });
});
