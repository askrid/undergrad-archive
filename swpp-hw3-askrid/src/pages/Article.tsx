import { useEffect } from "react";
import { useDispatch } from "react-redux";
import { useParams } from "react-router-dom";
import ArticleDetailView from "../components/ArticleDetailView";
import CommentList from "../components/CommentList";
import CreateCommentForm from "../components/CreateCommentForm";
import { AppDispatch } from "../store";
import { fetchSelectedArticle } from "../store/slices/article";
import { fetchCommentList } from "../store/slices/comment";

const Article = () => {
  const { articleId } = useParams();
  const dispatch = useDispatch<AppDispatch>();

  useEffect(() => {
    // TODO: handle not found error
    if (articleId) {
      dispatch(fetchSelectedArticle(parseInt(articleId)));
      dispatch(fetchCommentList(parseInt(articleId)));
    }
  }, [dispatch, articleId]);

  return (
    <main>
      <div className="mt-5 mb-8">
        <ArticleDetailView />
      </div>
      <div className="mb-8">
        <CommentList />
      </div>
      <div className="mb-16">
        <CreateCommentForm />
      </div>
    </main>
  );
};

export default Article;
