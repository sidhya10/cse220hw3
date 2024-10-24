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

void test_create_quadtree_detailed() {
    printf("\nTesting create_quadtree in detail...\n");
    
    prepare_input_image_file("building1.ppm");
    Image *image = load_image("images/building1.ppm");
    assert(image != NULL);
    
    // Test different RMSE values
    double rmse_values[] = {5.0, 25.0, 50.0, 100.0};
    for (int i = 0; i < 4; i++) {
        printf("Testing RMSE: %.1f\n", rmse_values[i]);
        QTNode *root = create_quadtree(image, rmse_values[i]);
        assert(root != NULL);
        
        // Basic node validation
        assert(root->width == get_image_width(image));
        assert(root->height == get_image_height(image));
        assert(root->row == 0);
        assert(root->col == 0);
        
        // Higher RMSE should result in fewer subdivisions
        if (i > 0) {
            // Save tree for comparison
            char filename[100];
            sprintf(filename, "tests/output/tree_rmse%.1f.txt", rmse_values[i]);
            save_preorder_qt(root, filename);
        }
        
        delete_quadtree(root);
    }
    
    // Test edge cases
    QTNode *null_result = create_quadtree(NULL, 25.0);
    assert(null_result == NULL);
    
    QTNode *zero_rmse = create_quadtree(image, 0.0);
    assert(zero_rmse != NULL);  // Should create maximum subdivision
    delete_quadtree(zero_rmse);
    
    delete_image(image);
    printf("create_quadtree tests passed!\n");
}

void test_save_preorder_detailed() {
    printf("\nTesting save_preorder_qt in detail...\n");
    
    prepare_input_image_file("building1.ppm");
    Image *image = load_image("images/building1.ppm");
    assert(image != NULL);
    
    QTNode *root = create_quadtree(image, 25.0);
    assert(root != NULL);
    
    // Test normal save
    save_preorder_qt(root, "tests/output/test_save1.txt");
    
    // Verify by loading and comparing
    QTNode *loaded_root = load_preorder_qt("tests/output/test_save1.txt");
    assert(loaded_root != NULL);
    
    // Save both trees and compare the files
    save_preorder_qt(root, "tests/output/original_tree.txt");
    save_preorder_qt(loaded_root, "tests/output/loaded_tree.txt");
    
    // Compare the files (they should be identical)
    FILE *f1 = fopen("tests/output/original_tree.txt", "r");
    FILE *f2 = fopen("tests/output/loaded_tree.txt", "r");
    assert(f1 && f2);
    
    char line1[256], line2[256];
    while (fgets(line1, sizeof(line1), f1) && fgets(line2, sizeof(line2), f2)) {
        assert(strcmp(line1, line2) == 0);
    }
    
    fclose(f1);
    fclose(f2);
    
    // Test edge cases
    save_preorder_qt(NULL, "tests/output/null_tree.txt");
    save_preorder_qt(root, NULL);
    
    delete_quadtree(root);
    delete_quadtree(loaded_root);
    delete_image(image);
    printf("save_preorder_qt tests passed!\n");
}

void test_hide_message_detailed() {
    printf("\nTesting hide_message in detail...\n");
    
    prepare_input_image_file("wolfie-tiny.ppm");
    
    // Test cases with different message lengths
    const char *test_messages[] = {
        "A",                    // Single character
        "Hello, World!",        // Standard message
        "",                     // Empty message
        "0000000000111111111122222222223333333333",  // Long message
        "Special chars: !@#$%^&*()"  // Special characters
    };
    
    for (int i = 0; i < 5; i++) {
        printf("Testing message: %s\n", test_messages[i]);
        
        char output_file[100];
        sprintf(output_file, "tests/output/hidden_msg%d.ppm", i);
        
        unsigned int chars_hidden = hide_message((char*)test_messages[i], 
                                               "images/wolfie-tiny.ppm",
                                               output_file);
                                               
        assert(chars_hidden <= strlen(test_messages[i]));
        
        // Reveal and verify
        char *revealed = reveal_message(output_file);
        assert(revealed != NULL);
        
        // Compare the revealed message with original (up to chars_hidden)
        assert(strncmp(revealed, test_messages[i], chars_hidden) == 0);
        
        free(revealed);
    }
    
    // Test edge cases
    unsigned int result;
    
    // NULL message
    result = hide_message(NULL, "images/wolfie-tiny.ppm", "tests/output/null_msg.ppm");
    assert(result == 0);
    
    // NULL input file
    result = hide_message("Test", NULL, "tests/output/null_input.ppm");
    assert(result == 0);
    
    // NULL output file
    result = hide_message("Test", "images/wolfie-tiny.ppm", NULL);
    assert(result == 0);
    
    // Non-existent input file
    result = hide_message("Test", "nonexistent.ppm", "tests/output/bad_input.ppm");
    assert(result == 0);
    
    printf("hide_message tests passed!\n");
}
static Image* create_test_image(unsigned short width, unsigned short height) {
    Image *img = malloc(sizeof(Image));
    if (!img) return NULL;
    
    img->width = width;
    img->height = height;
    img->pixels = malloc(width * height * sizeof(unsigned char));
    
    if (!img->pixels) {
        free(img);
        return NULL;
    }
    
    // Create checkerboard pattern to force quadtree subdivisions
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            if ((i/8 + j/8) % 2) {
                img->pixels[i * width + j] = 255;
            } else {
                img->pixels[i * width + j] = 0;
            }
        }
    }
    
    return img;
}

void test_quadtree_moderate() {
    printf("\nTesting quadtree with moderate cases...\n");
    
    // Test with different image sizes
    unsigned short test_sizes[][2] = {
        {64, 64},    // Small
        {128, 96},   // Medium
        {256, 192}   // Larger but still reasonable
    };
    
    for (int i = 0; i < 3; i++) {
        unsigned short width = test_sizes[i][0];
        unsigned short height = test_sizes[i][1];
        printf("Testing size %ux%u\n", width, height);
        
        Image *test_img = create_test_image(width, height);
        assert(test_img != NULL);
        
        // Test different RMSE values
        double rmse_values[] = {10.0, 25.0, 50.0};
        for (int j = 0; j < 3; j++) {
            printf("  Testing RMSE %.1f\n", rmse_values[j]);
            
            QTNode *root = create_quadtree(test_img, rmse_values[j]);
            assert(root != NULL);
            
            // Save tree and verify
            char filename[100];
            sprintf(filename, "tests/output/tree_%dx%d_rmse%.1f.txt", 
                    width, height, rmse_values[j]);
            save_preorder_qt(root, filename);
            
            // Load saved tree and verify
            QTNode *loaded = load_preorder_qt(filename);
            assert(loaded != NULL);
            
            // Compare trees by saving to PPM and comparing pixels
            save_qtree_as_ppm(root, "tests/output/original.ppm");
            save_qtree_as_ppm(loaded, "tests/output/loaded.ppm");
            
            Image *img1 = load_image("tests/output/original.ppm");
            Image *img2 = load_image("tests/output/loaded.ppm");
            assert(img1 && img2);
            
            // Verify images match
            assert(img1->width == img2->width);
            assert(img1->height == img2->height);
            for (int k = 0; k < img1->width * img1->height; k++) {
                assert(img1->pixels[k] == img2->pixels[k]);
            }
            
            delete_image(img1);
            delete_image(img2);
            delete_quadtree(root);
            delete_quadtree(loaded);
        }
        
        delete_image(test_img);
    }
    
    printf("Moderate quadtree tests passed!\n");
}

void test_steganography_moderate() {
    printf("\nTesting steganography with moderate cases...\n");
    
    // Test messages of different lengths
    const char *test_messages[] = {
        "This is a short message.",
        "This is a medium length message that should still fit easily.",
        "This is a longer message that will test the capacity of our steganography system. "
        "It includes multiple sentences and should be long enough to verify proper handling "
        "of larger amounts of text data."
    };
    
    prepare_input_image_file("building1.ppm");
    Image *cover = load_image("images/building1.ppm");
    assert(cover != NULL);
    
    for (int i = 0; i < 3; i++) {
        printf("Testing message %d (length: %zu)\n", i + 1, strlen(test_messages[i]));
        
        char outfile[100];
        sprintf(outfile, "tests/output/hidden_msg%d.ppm", i);
        
        unsigned int chars_hidden = hide_message((char*)test_messages[i], 
                                               "images/building1.ppm",
                                               outfile);
        printf("Characters hidden: %u\n", chars_hidden);
        assert(chars_hidden > 0);
        assert(chars_hidden <= strlen(test_messages[i]));
        
        char *revealed = reveal_message(outfile);
        assert(revealed != NULL);
        assert(strncmp(revealed, test_messages[i], chars_hidden) == 0);
        free(revealed);
    }
    
    // Test image steganography
    printf("\nTesting image hiding...\n");
    prepare_input_image_file("wolfie-tiny.ppm");
    
    unsigned int success = hide_image("images/wolfie-tiny.ppm",
                                    "images/building1.ppm",
                                    "tests/output/hidden_img_mod.ppm");
    assert(success == 1);
    
    reveal_image("tests/output/hidden_img_mod.ppm",
                 "tests/output/revealed_img_mod.ppm");
    
    // Verify revealed image
    Image *original = load_image("images/wolfie-tiny.ppm");
    Image *revealed = load_image("tests/output/revealed_img_mod.ppm");
    assert(original && revealed);
    assert(original->width == revealed->width);
    assert(original->height == revealed->height);
    
    delete_image(original);
    delete_image(revealed);
    delete_image(cover);
    
    printf("Moderate steganography tests passed!\n");
}

void test_preorder_output(QTNode *root, char *expected_filename) {
    // First save our tree
    save_preorder_qt(root, "tests/output/test_preorder.txt");
    
    // Open both files
    FILE *exp = fopen(expected_filename, "r");
    FILE *out = fopen("tests/output/test_preorder.txt", "r");
    if (!exp || !out) {
        printf("Failed to open files for comparison\n");
        if (exp) fclose(exp);
        if (out) fclose(out);
        return;
    }

    char exp_line[256], out_line[256];
    int line_num = 1;
    
    // Compare line by line
    while (fgets(exp_line, sizeof(exp_line), exp)) {
        if (!fgets(out_line, sizeof(out_line), out)) {
            printf("Output file is shorter than expected at line %d\n", line_num);
            break;
        }
        
        // Parse and compare each line
        char exp_type, out_type;
        unsigned int exp_i, exp_r, exp_h, exp_c, exp_w;
        unsigned int out_i, out_r, out_h, out_c, out_w;
        
        sscanf(exp_line, "%c %u %u %u %u %u", 
               &exp_type, &exp_i, &exp_r, &exp_h, &exp_c, &exp_w);
        sscanf(out_line, "%c %u %u %u %u %u", 
               &out_type, &out_i, &out_r, &out_h, &out_c, &out_w);
        
        // Compare values
        if (exp_type != out_type || exp_i != out_i || exp_r != out_r ||
            exp_h != out_h || exp_c != out_c || exp_w != out_w) {
            printf("Mismatch at line %d:\n", line_num);
            printf("Expected: %c %u %u %u %u %u\n", 
                   exp_type, exp_i, exp_r, exp_h, exp_c, exp_w);
            printf("Got:      %c %u %u %u %u %u\n", 
                   out_type, out_i, out_r, out_h, out_c, out_w);
        }
        
        line_num++;
    }
    
    // Check if output file is longer than expected
    if (fgets(out_line, sizeof(out_line), out)) {
        printf("Output file is longer than expected\n");
    }
    
    fclose(exp);
    fclose(out);
}

int main() {
    struct stat st;
    if (stat("tests/output", &st) == -1)
        mkdir("tests/output", 0700);


    // Test quadtree preorder save/load
    prepare_input_image_file("building1.ppm");
    Image *image = load_image("images/building1.ppm");
    QTNode *root = create_quadtree(image, 25);
    
    // Compare with expected output
    test_preorder_output(root, "tests/input/load_preorder_qt1_qtree.txt");

    // Test loading from file
    QTNode *loaded_root = load_preorder_qt("tests/input/load_preorder_qt1_qtree.txt");
    
    // Save loaded tree and compare again to verify loading worked correctly
    test_preorder_output(loaded_root, "tests/input/load_preorder_qt1_qtree.txt");

    delete_quadtree(root);
    delete_quadtree(loaded_root);
    delete_image(image);
    
    printf("Preorder tests completed\n");

    test_create_quadtree_detailed();
    test_save_preorder_detailed();
    test_hide_message_detailed();
    
    test_quadtree_creation();
    test_quadtree_io();
    test_steganography();

    test_quadtree_moderate();
    test_steganography_moderate();

    printf("\nAll tests completed successfully!\n");
    return 0;
}