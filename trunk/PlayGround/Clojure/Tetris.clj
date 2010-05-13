(import java.awt.Color)
(import java.awt.Dimension)
(import java.awt.Toolkit)
(import java.awt.event.KeyListener)
(import javax.swing.JFrame)
(import javax.swing.JPanel)

(use 'clojure.contrib.math)

(def prefs {
  :num-rows 20
  :num-columns 10
  :block-width 10
  :block-height 10
  :screen-width 320
  :screen-height 240
  :border-left 7
  :border-top 2
})

(def field
  (atom
    (vec
      (doall
        (for [_ (range (prefs :num-rows))]
          (vec (repeat (prefs :num-columns) 0)))))))

(defn get-field [rowIdx colIdx]
  (nth (nth @field rowIdx) colIdx))

(defn set-field [rowIdx colIdx value]
  (swap! field #(update-in % [rowIdx colIdx] (constantly value))))

(def i-block {
  :value 1
  :grids
  [
    [ [ 0 1 ]
      [ 0 1 ]
      [ 0 1 ]
      [ 0 1 ] ]    

    [ [ 0 0 0 0 ]
      [ 1 1 1 1 ] ]
  ]  
})

(def s-block {
  :value 2
  :grids
  [
    [ [ 0 2 2 ]
      [ 2 2 0 ] ] 
   
    [ [ 0 2 0 ]
      [ 0 2 2 ]
      [ 0 0 2 ] ]
  ]  
})

(def z-block {
  :value 3
  :grids
  [
    [ [ 3 3 0 ]
      [ 0 3 3 ] ]  
  
    [ [ 0 0 3 ]
      [ 0 3 3 ]
      [ 0 3 0 ] ]
  ]  
})

(def o-block {
  :value 4
  :grids
  [
    [ [ 0 0 0 ]
      [ 0 4 4 ]
      [ 0 4 4 ] ]
  ]  
})

(def t-block {
  :value 5
  :grids
  [
    [ [ 5 5 5 ]
      [ 0 5 0 ] ]
      
    [ [ 0 5 0 ]
      [ 5 5 0 ]
      [ 0 5 0 ] ]
      
    [ [ 0 5 0 ]
      [ 5 5 5 ] ]
      
    [ [ 0 5 0 ]
      [ 0 5 5 ]
      [ 0 5 0 ] ]
  ]  
})

(def l-block {
  :value 6
  :grids
  [
    [ [ 6 0 0 ]
      [ 6 0 0 ]
      [ 6 6 0 ] ]

    [ [ 0 0 0 ]
      [ 6 6 6 ]
      [ 6 0 0 ] ]

    [ [ 6 6 0 ]
      [ 0 6 0 ]
      [ 0 6 0 ] ]

    [ [ 0 0 0 ]
      [ 0 0 6 ]
      [ 6 6 6 ] ]
  ]
})

(def j-block {
  :value 7
  :grids
  [
    [ [ 0 7 0 ]
      [ 0 7 0 ]
      [ 7 7 0 ] ]

    [ [ 0 0 0 ]
      [ 7 0 0 ]
      [ 7 7 7 ] ]

    [ [ 7 7 0 ]
      [ 7 0 0 ]
      [ 7 0 0 ] ]

    [ [ 0 0 0 ]
      [ 7 7 7 ]
      [ 0 0 7 ] ]
  ]
})

(def active-block {
    :type (ref l-block)
    :rotation (ref 0)
    :rowIdx (ref 0)
    :colIdx (ref 0) })

(def block-types [i-block s-block z-block o-block t-block l-block j-block])

(defn random-block []
  (let [idx (mod (round (rand 1000)) (count block-types))]
    (nth block-types idx)))


  
(defn get-color [grid-value]
  (let [color-table [ Color/BLACK
                      Color/RED
                      Color/ORANGE
                      Color/YELLOW
                      Color/GREEN
                      Color/BLUE
                      (Color. 111 0 255)
                      (Color. 143 0 255)
                      Color/WHITE
                      Color/GRAY] ]
    (if (and (not (nil? grid-value)) (>= grid-value 0) (< grid-value (count color-table)))
      (nth color-table grid-value)
      (Color/WHITE))))


(defn center-in-screen [frame]
  (let [  dim (.getScreenSize(Toolkit/getDefaultToolkit))
          w (.width (.getSize frame))
          h (.height (.getSize frame))
          x 0 ; move window to the left of the monitor for easier debugging
          y (int (* 0.75(/ (- (.height dim) h) 2)))]
  (.setLocation frame x y)))

(defn draw-rectangle [g x y w h color]
  (doto g
    (.setColor color)
    (.fillRect (* w x) (* h y) w h)))

(defn draw-block [g block]
  (let [  block-type  (deref (block :type))
          row      	  (deref (block :rowIdx))
          col         (deref (block :colIdx))
          rotation    (deref (block :rotation))
          grids       (block-type :grids)
          grid-idx    (mod rotation (count grids))
          rows        (nth grids grid-idx)
          num-rows    (count rows)]
  (dotimes [ri num-rows]
    (let [current-row   (nth rows ri)
          num-columns   (count current-row)          ]
      (dotimes [ci num-columns]
        (let [cell-value (nth current-row ci)]
          (if-not (zero? cell-value)
            (let [x	(+ (prefs :border-left) (+ col ci))
                  y	(+ (prefs :border-top) (+ row ri))]
            (draw-rectangle g x y
                            (prefs :block-width)
                            (prefs :block-height)
                            (get-color cell-value))))))))))

(defn draw-field [g field]
  (dotimes [ rowIdx (count @field) ]
    (let [current-row (nth @field rowIdx)
          num-cols    (count current-row) ]    
      (dotimes [ colIdx num-cols ]
        (draw-rectangle g (+ (prefs :border-left) colIdx)
                          (+ (prefs :border-top) rowIdx)
                          (prefs :block-width)
                          (prefs :block-height)
                          (get-color (current-row colIdx)))))))

(defn draw-all [g]
  (let [ b active-block ]
    (draw-field g field)
    (draw-block g b)))
    
(defn rotate [block]
  (dosync (alter (block :rotation) inc)))
  

(defn next-block []
  (dosync (alter (active-block :type) (fn [oldBlock] (random-block)))))
  
(defn first-non-zero-element [row]
  (count (take-while #(zero? %) row)))

(defn first-if [collection predicate]
  (count (take-while (fn [n] (not (predicate n))) collection)))
  
(defn last-if [collection predicate]
  (- (dec (count collection)) (first-if (rseq collection) predicate)))

(defn max-x [b]
  (let [block-type  (deref (b :type))
        rotation    (deref (b :rotation))
        grids       (block-type :grids)
        grid-idx    (mod rotation (count grids))
        rows        (nth grids grid-idx) ]
    (last-if
      (reduce 
        (fn [r1 r2]
          (let [lastNonZeroValue (fn [row] (last-if row (comp not zero?)))]
            (if (> (lastNonZeroValue r1) (lastNonZeroValue r2)) r1 r2)))
        rows)
      (comp not zero?))))

(defn max-y [b]
  (let [block-type  (deref (b :type))
        rotation    (deref (b :rotation))
        grids       (block-type :grids)
        grid-idx    (mod rotation (count grids))
        rows        (nth grids grid-idx) ]
    (- (dec (count rows)) (count (take-while (fn [row] (zero? (reduce max row))) (rseq rows))))))

(defn min-x [b]
  (let [block-type  (deref (b :type))
        rotation    (deref (b :rotation))
        grids       (block-type :grids)
        grid-idx    (mod rotation (count grids))
        rows        (nth grids grid-idx) ]
    (* -1 (first-non-zero-element
      (reduce (fn [r1 r2]
                (if (< (first-non-zero-element r1)
                       (first-non-zero-element r2))
  			      r1 r2))
              rows)))))

(defn move-left [b]
  (if (< (min-x b) (deref (b :colIdx)))
      (dosync (alter (b :colIdx) dec))))

(defn move-right [b]
  (if (< (+ (deref (b :colIdx)) (max-x b)) (dec (prefs :num-columns)))
    (dosync (alter (b :colIdx) inc))))

(defn commit-block [block]
  (let [  block-type  (deref (block :type))
          rowIdx   	  (deref (block :rowIdx))
          colIdx      (deref (block :colIdx))
          rotation    (deref (block :rotation))
          grids       (block-type :grids)
          grid-idx    (mod rotation (count grids))
          rows        (nth grids grid-idx)
          num-rows    (count rows)]
  (dotimes [ri num-rows]
    (let [current-row   (nth rows ri)
          num-columns   (count current-row)          ]
      (dotimes [ci num-columns]
        (let [cell-value (nth current-row ci)]
          (if-not (zero? cell-value)
            (let [c	(+ (prefs :border-left) (+ colIdx ci))
                  r	(+ (prefs :border-top) (+ rowIdx ri))]
              (set-field r c cell-value)))))))))
    
(defn move-down [b]
  (if (< (+ (deref (b :rowIdx)) (max-y b)) (dec (prefs :num-rows)))
    (dosync (alter (b :rowIdx) inc))
    (commit-block b)))

(defn create-panel []
  (doto
    (proxy [JPanel KeyListener] []
      (paintComponent [g]
        (proxy-super paintComponent g)
        (draw-all g))
      (getPreferredSize [] (Dimension. 320 240))
      (keyPressed [e]
        (let [keyCode (.getKeyCode e)]
          (if (== 37 keyCode) (move-left active-block)
          (if (== 38 keyCode) (rotate active-block)
          (if (== 39 keyCode) (move-right active-block)
          (if (== 40 keyCode) (move-down active-block)
          (if (== 32 keyCode) (next-block)
                              (println keyCode))))))))
      (keyReleased [e])
      (keyTyped [e]))
    (.setFocusable true)))

(def panel
  (create-panel))

(defn main []
  (let [frame (JFrame. "Test")]
    (doto frame
      (.add panel)
      (.pack)
      (.setDefaultCloseOperation JFrame/EXIT_ON_CLOSE)    
      (.setVisible true))
    (.addKeyListener panel panel)
    (center-in-screen frame)
    (loop []
      (.repaint panel)
      (Thread/sleep 100)
      (recur))))

;(main)
