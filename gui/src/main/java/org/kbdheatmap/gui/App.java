package org.kbdheatmap.gui;

import java.io.InputStream;
import java.text.MessageFormat;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.NoSuchElementException;

import javafx.application.Application;
import javafx.geometry.Insets;
import javafx.scene.Scene;
import javafx.scene.control.Button;
import javafx.scene.input.KeyEvent;
import javafx.scene.layout.HBox;
import javafx.scene.layout.VBox;
import javafx.stage.Stage;

public class App extends Application {

    // Gruvbox colors
    private static final double[] BACKGROUND_COLOR = { 28, 28, 28 };
    private static final double[] HOT_COLOR = { 204, 26, 29 };
    private static final double[] COLD_COLOR = { 69, 133, 136 };

    // Keyboard layout in evdev-keycodes by row format
    private static final String LAYOUT_FILE = "/layout.txt";
    // Result of the keylogger program, format: keycode,num_presses,time
    private static final String KEYLOG_FILE = "/keylog.csv";
    /** 
      * Conversion from evdev keycode to javafx keycode
      * Format: evdev-code javafx-code representation width
      */
    private static final String EVDEV_KEYMAP_FILE = "/evdev2javafx.csv";

    // Global flag for input freeze
    private Boolean isInputFrozen = true;

    // List of all keys read from EVDEV_KEYMAP_FILE
    private List<KeyEntry> keys = new ArrayList<>();

    @Override
    public void start(Stage stage) {
        VBox root = new VBox();
        root.setPadding(new Insets(10));
        root.setSpacing(10);
        root.setStyle(
            MessageFormat.format("-fx-background-color: rgb({0}, {1}, {2});",
            BACKGROUND_COLOR[0], BACKGROUND_COLOR[1], BACKGROUND_COLOR[2]));

        readEvdevJavafxMap();
        List<List<KeyEntry>> keyboard = readKbdLayout();
        List<KbdButton> buttons = setKbdGridLayout(root, keyboard);
        setControls(root, buttons);
        Scene scene = new Scene(root);
        scene.addEventHandler(KeyEvent.KEY_PRESSED, event -> {
            handleKeyPressed(buttons, event);
            redraw(buttons);
        });
        stage.setScene(scene);
        stage.setTitle("Keyboard Heatmap");
        stage.show();
    }

    /**
      * Read the mapping of evdev keycodes to javafx keycodes
      * Format: evdev-code javafx-code representation
      * Modifies {@code keys} 
      */
    private void readEvdevJavafxMap() {
        List<String> lines = readLines(EVDEV_KEYMAP_FILE);
        for (String line : lines) {
            String[] split = line.split(" ");
            KeyEntry entry = new KeyEntry(split[0], split[1], split[2],
                Integer.parseInt(split[3]));
            keys.add(entry);
        }
    }

    /**
      * Add control buttons to GUI
      * @param root the root node containing keyboard heatmap
      * @param buttons the list of keyboard buttons
      */
    private void setControls(VBox root, List<KbdButton> buttons) {
        Button resetButton = new Button("Reset");
        Button freezeButton =
            new Button(isInputFrozen ? "Unfreeze" : "Freeze");
        Button loadButton = new Button("Load");
        resetButton.setFocusTraversable(false);
        freezeButton.setFocusTraversable(false);
        loadButton.setFocusTraversable(false);
        resetButton.setOnAction(event -> {
            buttons.stream()
                .forEach(button -> button.setCounter(0));
            redraw(buttons);
        });
        freezeButton.setOnAction(event -> {
            isInputFrozen = !isInputFrozen;
            freezeButton.setText(isInputFrozen ? "Unfreeze" : "Freeze");
        });
        loadButton.setOnAction(event -> {
            setInitialHeatmap(buttons);
        });
        HBox controls = new HBox();
        controls.setSpacing(10);
        controls.setPadding(new Insets(10));
        controls.getChildren().addAll(resetButton, freezeButton, loadButton);
        root.getChildren().add(controls);
    }

    
    /**
      * Set the layout of the keyboard
      * @param root the root node containing keyboard heatmap
      * @param keyboard the keyboard layout
      */
    private List<KbdButton> setKbdGridLayout(VBox root,
            List<List<KeyEntry>> keyboard) {
        List<KbdButton> buttons = new ArrayList<>();
        for (int i = 0; i < keyboard.size(); i++) {
            HBox row = new HBox();
            for (int j = 0; j < keyboard.get(i).size(); j++) {
                KbdButton button = new KbdButton(keyboard.get(i).get(j));
                button.setStyle("-fx-background-color: rgb(" +
                    COLD_COLOR[0] + ", " + COLD_COLOR[1] + ", " +
                    COLD_COLOR[2] + ");");
                buttons.add(button);
                row.getChildren().add(button);
            }
            root.getChildren().add(row);
        }
        return buttons;
    }

    /**
      * Handle a key pressed event
      * @param buttons the list of keyboard buttons to modify
      * @param event the key event
      */
    private void handleKeyPressed(List<KbdButton> buttons, KeyEvent event) {
        if (isInputFrozen) {
            return;
        }
        String pressed = event.getCode().toString();
        buttons.stream()
            .filter(button -> button.getEntry().javafxCode().equals(pressed))
            .forEach(KbdButton::click);
    }

    /**
      * Read lines from a file
      * @param path the path to the resource
      * @return the list of lines
      */
    private List<String> readLines(String path) {
        List<String> lines = new ArrayList<>();
        try (InputStream inputStream =
                getClass().getResourceAsStream(path)) {
            int c;
            StringBuilder sb = new StringBuilder();
            while ((c = inputStream.read()) != -1) {
                if (c == '\n') {
                    lines.add(sb.toString());
                    sb = new StringBuilder();
                } else {
                    sb.append((char) c);
                }
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
        return lines;
    }

    /**
      * Update keyboard heatmap with values from {@code KEYLOG_FILE}
      * @param buttons the list of keyboard buttons
      */
    private void setInitialHeatmap(List<KbdButton> buttons) {
        List<String> lines = readLines(KEYLOG_FILE);
        for (String line : lines) {
            String[] split = line.split(",");
            if (keys.stream()
                    .anyMatch(key -> key.evdevCode().equals(split[0]))) {
                int counter = Integer.parseInt(split[1]);
                buttons.stream()
                    .filter(button ->
                        button.getEntry().evdevCode().equals(split[0]))
                    .forEach(button -> button.setCounter(counter));
            }
        }
        redraw(buttons);
    }

    /**
      * Read keyboard layout from {@code LAYOUT_FILE}
      * @return the keyboard layout
      */
    private List<List<KeyEntry>> readKbdLayout() {
        List<String> lines = readLines(LAYOUT_FILE);
        List<List<KeyEntry>> keyboard = new ArrayList<>();
        try {
            for (String line : lines) {
                keyboard.add(Arrays.stream(line.split(" "))
                    .map(s -> keys.stream()
                        .filter(key -> key.evdevCode().equals(s))
                        .findFirst().orElseThrow())
                    .toList());
            }
        } catch (NoSuchElementException e) {
            // TODO: provide default layout and show a warning
            for (String line : lines) {
                for (String s : line.split(" ")) {
                    if (!keys.stream()
                            .anyMatch(key -> key.evdevCode().equals(s))) {
                        System.err.println("Missing key: " + s);
                    }
                }
            }
            keyboard = new ArrayList<>();
        }
        return keyboard;
    }

    /**
      * Update keyboard styles
      * @param buttons the list of keyboard buttons
      */
    private void redraw(List<KbdButton> buttons) {
        int maxPressed = buttons.stream()
            .mapToInt(KbdButton::getCounter)
            .max()
            .orElse(1);
        buttons.stream()
            .forEach(button -> {
                button.setStyle("-fx-background-color: " +
                    getStyle(maxPressed, button.getCounter()));
            });
    }

    /**
      * Get a CSS style string
      * @param maxPressed the maximum number of presses
      * @param counter the current number of presses
      * @return the CSS style string
      */
    private String getStyle(int maxPressed, int counter) {
        maxPressed = Math.max(maxPressed, 1);
        double percent = (double) counter / maxPressed;
        int[] color = getColorGradient(percent);
        return MessageFormat.format("rgb({0}, {1}, {2})",
            (int) color[0], (int) color[1], (int) color[2]);
    }

    /**
      * Calculate color gradient
      * @param percent the percentage of the gradient
      * @return the RGB color
      */
    private int[] getColorGradient(double percent) {
        double[] cold = COLD_COLOR;
        double[] hot = HOT_COLOR;
        double distance = Math.pow(cold[0] - hot[0], 2);
        distance += Math.pow(cold[1] - hot[1], 2);
        distance += Math.pow(cold[2] - hot[2], 2);
        distance = Math.sqrt(distance);
        double[] direction = {
            (hot[0] - cold[0]) / distance,
            (hot[1] - cold[1]) / distance,
            (hot[2] - cold[2]) / distance
        };
        double hotness = distance * percent;
        int[] color = {
            (int) (cold[0] + hotness * direction[0]),
            (int) (cold[1] + hotness * direction[1]),
            (int) (cold[2] + hotness * direction[2])
        };
        return color;
    }

    public static void main(String[] args) {
        launch(args);
    }
}
