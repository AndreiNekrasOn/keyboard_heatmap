package org.kbdheatmap.gui;

import javafx.geometry.Pos;
import javafx.scene.layout.StackPane;
import javafx.scene.paint.Color;
import javafx.scene.shape.Rectangle;
import javafx.scene.shape.StrokeLineCap;
import javafx.scene.shape.StrokeLineJoin;
import javafx.scene.shape.StrokeType;
import javafx.scene.text.Text;

public class KbdButton extends StackPane {

    // Rectangle base size
    private static final int SIZE = 50;
    private static final int STROKE_WIDTH = 3;

    // Base info
    private KeyEntry entry;
    // Number of presses
    private int counter = 0;

    public KbdButton(KeyEntry entry) {
        this.entry = entry;

        Rectangle rect = new Rectangle(SIZE * entry.width(), SIZE);
        rect.setFill(Color.TRANSPARENT);
        rect.setStroke(Color.BLACK);
        rect.setStrokeType(StrokeType.INSIDE);
        rect.setStrokeLineCap(StrokeLineCap.ROUND);
        rect.setStrokeLineJoin(StrokeLineJoin.ROUND);
        rect.setStrokeWidth(STROKE_WIDTH);
        Text text = new Text(entry.repr());
        text.getStyleClass().add("kbd-text");
        getChildren().addAll(rect, text);
        setAlignment(Pos.CENTER);
    }

    public String getText() {
        return ((Text) getChildren().get(1)).getText();
    }

    public void click() {
        counter++;
    }

    public int getCounter() {
        return counter;
    }

    public void setCounter(int counter) {
        this.counter = counter;
    }

    public KeyEntry getEntry() {
        return entry;
    }
}
