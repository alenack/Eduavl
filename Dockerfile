# ---- build stage ----
FROM gcc:13 AS build
WORKDIR /src
COPY avl.c avl.h server.c ./
RUN gcc -O2 -Wall avl.c server.c -o server -lpthread

# ---- runtime stage (small image) ----
FROM debian:bookworm-slim
WORKDIR /app
COPY --from=build /src/server ./server
COPY public ./public
RUN useradd -r -u 10001 appuser && chown -R appuser /app
USER appuser
ENV PORT=8080
EXPOSE 8080
CMD ["./server"]
