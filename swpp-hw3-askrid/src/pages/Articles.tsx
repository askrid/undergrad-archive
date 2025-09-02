import { useEffect } from "react";
import { useDispatch } from "react-redux";
import ArticleList from "../components/ArticleList";
import GoToArticleCreateButton from "../components/buttons/GoToArticleCreateButton";
import { AppDispatch } from "../store";
import { fetchArticleList } from "../store/slices/article";

const Articles = () => {
  const dispatch = useDispatch<AppDispatch>();

  useEffect(() => {
    dispatch(fetchArticleList());
  }, [dispatch]);

  return (
    <main>
      <div className="flex justify-between items-center mt-10 mb-4">
        <h1 className="font-semibold text-2xl">All Articles</h1>
        <GoToArticleCreateButton />
      </div>
      <div className="mb-16">
        <ArticleList />
      </div>
    </main>
  );
};

export default Articles;
