import axios from "axios";

/**
 * Use this to avoid duplicate network call. Note that this returns a synchronous function.
 */
export const useCachedFetcher = <Data>() => {
  const cache = new Map<string, Promise<Data>>();

  const fetcher = (url: string): Promise<Data> => {
    let promise = cache.get(url);

    if (!promise) {
      promise = axios.get<Data>(url).then((res) => res.data);
      cache.set(url, promise);
    }

    return promise;
  };

  return fetcher;
};

export const NotLoggedInError = new Error("Not logged in");
