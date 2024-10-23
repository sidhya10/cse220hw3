#include "qtree.h"
#include "image.h"
#include "tests_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Helper function to compare images
static int compare_images(Image *img1, Image *img2) {
    if (!img1 || !img2) return 0;
    if (get_image_width(img1) != get_image_width(img2) || 
        get_image_height(img1) != get_image_height(img2)) return 0;
    
    for (unsigned int i = 0; i < get_image_height(img1); i++) {
        for (unsigned int j = 0; j < get_image_width(img1); j++) {
            if (get_image_intensity(img1, i, j) != get_image_intensity(img2, i, j)) 
                return 0;
        }
    }
    return 1;
}

// Test quadtree creation and basic properties
void test_quadtree_creation() {
    printf("Testing quadtree creation...\n");
    
    prepare_input_image_file("building1.ppm");
    Image *image = load_image("images/building1.ppm");
    assert(image != NULL);
    
    double max_rmse = 25;
    QTNode *root = create_quadtree(image, max_rmse);
    assert(root != NULL);
    
    // Test basic node properties
    assert(get_node_intensity(root) >= 0);
    assert(get_node_intensity(root) <= 255);
    
    // Test child access functions
    QTNode *child1 = get_child1(root);
    QTNode *child2 = get_child2(root);
    QTNode *child3 = get_child3(root);
    QTNode *child4 = get_child4(root);
    
    // At least one child should exist for this image
    assert(child1 != NULL || child2 != NULL || child3 != NULL || child4 != NULL);
    
    delete_quadtree(root);
    delete_image(image);
    printf("Quadtree creation tests passed!\n");
}

// Test quadtree file I/O
void test_quadtree_io() {
    printf("Testing quadtree I/O...\n");
    
    // Create a quadtree
    Image *image = load_image("images/building1.ppm");
    QTNode *root = create_quadtree(image, 25);
    
    // Test save_preorder_qt
    save_preorder_qt(root, "tests/output/save_preorder_qt1_qtree.txt");
    
    // Load the saved quadtree
    QTNode *loaded_root = load_preorder_qt("tests/output/save_preorder_qt1_qtree.txt");
    assert(loaded_root != NULL);
    
    // Save both trees as PPM and compare
    save_qtree_as_ppm(root, "tests/output/original_qt.ppm");
    save_qtree_as_ppm(loaded_root, "tests/output/loaded_qt.ppm");
    
    Image *original_img = load_image("tests/output/original_qt.ppm");
    Image *loaded_img = load_image("tests/output/loaded_qt.ppm");
    
    assert(compare_images(original_img, loaded_img));
    
    delete_quadtree(root);
    delete_quadtree(loaded_root);
    delete_image(image);
    delete_image(original_img);
    delete_image(loaded_img);
    printf("Quadtree I/O tests passed!\n");
}

// Test steganography functions
void test_steganography() {
    printf("Testing steganography...\n");
    
    // Test message hiding/revealing
    prepare_input_image_file("wolfie-tiny.ppm");
    const char *test_message = "0000000000111111111122222222223333333333";
    
    // First check if image can hold the message
    Image *test_img = load_image("images/wolfie-tiny.ppm");
    assert(test_img != NULL);
    
    size_t max_chars = (test_img->width * test_img->height) / 8 - 1;
    printf("Image can hold up to %zu characters\n", max_chars);
    printf("Test message length: %zu characters\n", strlen(test_message));
    
    unsigned int expected_chars = (strlen(test_message) < max_chars) ? 
                                 strlen(test_message) : max_chars;
    
    unsigned int chars_hidden = hide_message((char*)test_message, 
                                           "images/wolfie-tiny.ppm",
                                           "tests/output/hide_message1.ppm");
    
    printf("Characters hidden: %u\n", chars_hidden);
    printf("Expected characters: %u\n", expected_chars);
    assert(chars_hidden == expected_chars);
    
    char *revealed = reveal_message("tests/output/hide_message1.ppm");
    assert(revealed != NULL);
    
    // Compare only the number of characters that were actually hidden
    assert(strncmp(revealed, test_message, chars_hidden) == 0);
    free(revealed);
    delete_image(test_img);
    
    // Test image hiding/revealing
    prepare_input_image_file("building1.ppm");
    prepare_input_image_file("wolfie-tiny.ppm");
    
    unsigned int success = hide_image("images/wolfie-tiny.ppm",
                                    "images/building1.ppm",
                                    "tests/output/hide_image1.ppm");
    assert(success == 1);
    
    reveal_image("tests/output/hide_image1.ppm",
                 "tests/output/reveal_image1.ppm");
    
    printf("Steganography tests passed!\n");
}

int main() {
    // Create output directory if needed
    struct stat st;
    if (stat("tests/output", &st) == -1)
        mkdir("tests/output", 0700);
        
    // Run all tests
    test_quadtree_creation();
    test_quadtree_io();
    test_steganography();
    
    printf("\nAll tests completed successfully!\n");
    return 0;
}