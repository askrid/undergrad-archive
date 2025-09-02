import { useEffect } from "react";
import { useDispatch, useSelector } from "react-redux";
import { useParams } from "react-router-dom";
import ArticleDetailPreview from "../components/ArticleDetailPreview";
import ArticleForm from "../components/ArticleForm";
import ArticleEditConfirmButton from "../components/buttons/ArticleEditConfirmButton";
import ArticleViewSelectButtons from "../components/buttons/ArticleViewSelectButtons";
import BackToArticleButton from "../components/buttons/BackToArticleButton";
import { AppDispatch } from "../store";
import { articleActions, fetchSelectedArticle, selectArticle } from "../store/slices/article";

const ArticleEdit = () => {
  const { articleId } = useParams();
  const dispatch = useDispatch<AppDispatch>();
  const { selectedArticle, writingArticleView } = useSelector(selectArticle);

  useEffect(() => {
    dispatch(articleActions.setWritingArticleView("WRITE"));
  }, [dispatch]);

  useEffect(() => {
    if (articleId) {
      dispatch(fetchSelectedArticle(parseInt(articleId)));
    }
  }, [dispatch, articleId]);

  useEffect(() => {
    if (selectedArticle) {
      dispatch(articleActions.writeArticleTitle(selectedArticle.title));
      dispatch(articleActions.writeArticleContent(selectedArticle.content));
    }
  }, [dispatch, selectedArticle]);

  return (
    <main>
      <div className="font-semibold flex justify-between items-center mt-5 mb-4">
        <ArticleViewSelectButtons />
        <BackToArticleButton />
      </div>
      <div className="mb-4">
        {writingArticleView === "WRITE" ? <ArticleForm /> : <ArticleDetailPreview />}
      </div>
      <div className="mb-16">
        <ArticleEditConfirmButton />
      </div>
    </main>
  );
};

export default ArticleEdit;
